import types, sys, math, time, os, getopt

import Pmw
import Tkinter as Tk
import tkFont
import SlSoggy

import traceback, sys

import dobj
import system
import parameters
import colors
import anim
import controller
import draw
import library
import error

timeout_interval = 15

allowed_keyevents = 'abcdefghijklmnopqrstuvwxyz0123456789'

class ObjectViewer:
    def __init__( self, root, objectlist, screen_size, clear_color, visual, timeout = timeout_interval ):
        self.process_objectlist( objectlist )

        self.clear_color = clear_color
        self.root = root
        self.size = None
        self.idle_id = None

        self.preload_done = 0

        self.timeout_interval = timeout

        self.index = None
        self.object = None
        self.pdict = {}

        self.controls_shown = 1

        self.marklist = None
        self.markmode = 0
        self.markmenu = None
        self.current_mark = None

        self.savenum = 0
        self.savefilename = None
        
        self.labelfont = tkFont.Font( family='Helvetica', size=8 )

        self.read_callback = None

        mb = self.menubar = Pmw.MainMenuBar( root )
        mb.addmenu( 'File', '' )
        mb.addmenuitem( 'File', 'command', label='Save screen', command = self.save_screen )
        mb.addmenuitem( 'File', 'command', label='Quit', command = self.quit )

        mb.addmenu( 'Object', '' )
        self.build_object_menu()

        self.panes = panes = Pmw.PanedWidget( root, command = self.resize_panes,
                                 hull_width = screen_size[0], hull_height = int(screen_size[1] * 1.25)+5)
        pane0 = panes.add( 'soggy', size = 0.8 )
        pane1 = panes.add( 'coord', size = 0.2 )

        try:
            if visual:
                d = system.parse_visual_arg( visual )
            else:
                d = {}

            self.s = SlSoggy.Soggy( pane0, width = screen_size[0], height = screen_size[1],
                                    init = dobj.init, reshape = self.reshape, redraw = self.redraw,
                                    **d )
        except (Tk.TclError, ValueError), msg:
            print msg
            sys.exit(1)

        self.show_soggy_visual()
            
        library._reader.set_doquery( self.eval_query )

        self.coords = Tk.Frame( pane1, height = 30 )
        self.coords_label = Tk.Label( self.coords )
        self.coords_val = Tk.Label( self.coords, width = 20 )
        self.coords_label.pack( expand = 1, fill = 'both', side = 'left' )
        self.coords_val.pack( fill = 'y', side = 'right' )

        self.sframe = sframe = Pmw.ScrolledFrame( pane1, usehullsize = 1,
                                    horizflex = 'expand',
                                    #vertflex = 'elastic',
                                    hscrollmode = 'none' )
        self.controls = sframe.component( 'frame' )

        sframe.pack( fill='x', padx=2, pady=2, side = Tk.BOTTOM )
        self.coords.pack( fill = 'x', padx=2, pady=2, side = Tk.BOTTOM )
        Tk.Frame( pane0, height = 4 ).pack( side = Tk.BOTTOM )
        self.s.pack( expand = 1, fill = 'both', side = Tk.BOTTOM )

        panes.pack( expand = 1, fill = 'both' )
        
        root.bind( '<Key-Escape>', self.quit )
        self.s.bind( '<Key-period>', lambda *x: self.set_object_rel(1) )
        self.s.bind( '<Key-comma>', lambda *x: self.set_object_rel(-1) )
        self.s.bind( '<Key-equal>', self.save_screen )
        root.bind( '<Key-Tab>', self.toggle_controls )

        self.s.bind( '<Control-ButtonPress-3>', self.select_mark_menu )
        self.s.bind( '<ButtonPress-3>', self.show_coords )
        
        self.s.bind( '<KeyPress>', self.event_key )
        self.s.bind( '<ButtonPress-1>', self.event_mousedown )
        
        root.configure( menu = self.menubar )

        if self.objectlist:
            self.set_object( 0 )
        self.s.focus()

    def show_soggy_visual( self ):
        r = self.s.cget( 'red' )
        g = self.s.cget( 'green' )
        b = self.s.cget( 'blue' )
        a = self.s.cget( 'alpha' )
        depth = self.s.cget( 'depth' )
        stencil = self.s.cget( 'stencil' )

        print '     visual : %s/%s/%s/%s/%s/%s' % (r, g, b, a, depth, stencil)

    def resize_panes( self, sizelist ):
        h = sizelist[1] - int(self.coords.cget('height'))
        self.sframe.component('hull').config( height = h )

    def quit( self, *ev ):
        self.root.quit()

    def save_screen( self, *ev ):
        self.savefilename = 'slithy%04d.png' % (self.savenum,)
        self.savenum += 1
        self.s.redraw()
        

    def reshape( self, w, h ):
        self.size = (int(w), int(h))
        self.aspect = float(w) / float(h)
        dobj.reshape( *self.size )

    def redraw( self ):
        if self.size is None:
            dobj.blank()
            return

        try:
            self.marklist = draw._save_marked_cameras( self.markmode )
            self.markmode = 0

            library._reader.mode = 1

            dobj.startdraw( self.clear_color )
            draw._frame()
            system.global_draw_object( self.object, self.pdict, self.aspect, 1.0 )
            if self.savefilename:
                dobj.enddraw( self.savefilename )
                self.savefilename = None
            else:
                dobj.enddraw()

            library._reader.mode = 0
        except KeyboardInterrupt:
            self.quit()
        except:
            traceback.print_exc()
            sys.exit(1)
            
            

    def eval_query( self, xy ):
        self.s.eval( lambda: self.query_redraw( xy ) )
        return self.lastqueryresult

    def query_redraw( self, xy ):
        library._reader.mode = 1
        
        draw._save_marked_cameras( 0 )
        dobj.startdraw( self.clear_color, xy )
        draw._frame()
        system.global_draw_object( self.object, self.pdict, self.aspect, 1.0 )
        self.lastqueryresult = dobj.enddraw()
        
        library._reader.mode = 0

    def preload_images( self, item ):
        if hasattr( item, 'itervalues' ):
            for j in item.itervalues():
                self.preload_images( j )
            return

        try:
            x = iter(item)
            for j in x:
                self.preload_images( j )
        except TypeError:
            item.preload()

    def show_coords( self, ev ):
        self.get_marklist()

        if self.current_mark:
            x, y = draw._unproject( ev.x, self.size[1] - ev.y, self.marklist[0], self.current_mark[1] )
            self.coords_val.config( text = '(%.3f, %.3f)' % (x, y) )

    def select_mark_menu( self, ev ):
        self.get_marklist()
            
        if self.markmenu is None:
            self.markmenu = Tk.Menu( self.s, tearoff = 0 )
            for item in self.marklist[1:]:
                self.markmenu.add_command( label = item[0], command = lambda i=item: self.select_mark(i) )

        self.markmenu.tk_popup( ev.x_root, ev.y_root )

    def get_marklist( self ):
        if self.marklist is None:
            self.markmode = 1
            self.s.redraw()
            self.s.update_idletasks()

            if self.markmenu is not None:
                self.markmenu.destroy()
                self.markmenu = None

            mark = None

            if self.current_mark is not None:
                for i in self.marklist[1:]:
                    if i[0] == self.current_mark[0]:
                        mark = i
                        break
                    
            if mark is None and len(self.marklist) > 1:
                mark = self.marklist[1]
                
            self.select_mark( mark )

    def select_mark( self, mark = None ):
        self.current_mark = mark

        if mark is None:
            self.coords_label.config( text = '<none>' )
        else:
            self.coords_label.config( text = mark[0] )

    def diagram_eval( self, *junk ):
        for k, v in self.tkvars.iteritems():
            if isinstance( v, tuple ):
                self.pdict[k] = colors.Color( v[0].get(), v[1].get(), v[2].get(), v[3].get() )
            else:
                self.pdict[k] = v.get()
        self.marklist = None
        self.s.redraw()
        self.s.focus()

    def anim_eval( self, *junk ):
        self.pdict['t'] = self.tkvars.get()
        self.marklist = None
        self.s.redraw()
    
    def set_object_rel( self, offset ):
        n = len(self.objectlist)
        k = (self.index + offset + n) % n
        self.set_object( k )

    def set_object( self, index ):
        object, pdict, name = self.objectlist[index]
        self.root.wm_title( 'Slithy object tester [%s]' % (name,) )
        self.marklist = None

        if isinstance( self.object, anim.Animation ) and isinstance( object, anim.Animation ):
            self.retarget_animation_controls( index, object, pdict )
            self.anim_eval()
            self.get_marklist()
            return

        if isinstance( self.object, anim.Animation ):
            self.destroy_animation_controls()
        elif isinstance( self.object, controller.Controller ):
            self.destroy_controller_controls()
        elif isinstance( self.object, tuple ) or callable( self.object ):
            self.destroy_diagram_controls()
        elif self.object is None:
            pass
        else:
            raise RuntimeError, "this should not have happened."

        if isinstance( object, anim.Animation ):
            self.build_animation_controls( index, object, pdict )
            self.anim_eval()
        elif isinstance( object, type ) and issubclass( object, controller.Controller ):
            self.build_controller_controls( index, object, pdict )
            self.marklist = None
            self.pdict['t'] = 0.0
            self.s.redraw()
        elif isinstance( object, tuple ) or callable( object ):
            self.build_diagram_controls( index, object, pdict )
            self.diagram_eval()
        else:
            raise ValueError, "set_object() called with bad drawing object"

        self.root.update()

    def retarget_animation_controls( self, index, object, pdict ):
        self.stop_play()
        
        self.object = object
        self.index = index
        self.pdict = { 't' : 0.0 }

        self.wcontrols[0].config( to = object.length )
        self.tkvars.set( 0.0 )
        

    def build_controller_controls( self, index, object, pdict ):
        self.object = object()
        self.objectclass = object
        self.index = index
        self.pdict = { 't' : 0.0 }

        self.tkvars = None

        w = Tk.Frame( self.controls )
        w.grid( row = 1, column = 0, sticky = Tk.EW, padx = 5 )
        self.wcontrols = [w]

        b = Tk.Button( w, text = 'reset', command = self.reset_play )
        b.pack( side = 'left' )

        self.controls.grid_columnconfigure( 0, weight = 1 )
        self.controls.grid_columnconfigure( 1, weight = 0 )

        self.abind = self.root.bind( '<Key-space>', lambda *x: self.start_play( advance = 1 ) )

        self.start_play()

    def destroy_controller_controls( self ):
        self.stop_play()
        del self.tkvars
        for i in self.wcontrols:
            i.destroy()
        del self.wcontrols
        del self.objectclass

        self.root.unbind( '<Key-space>', self.abind )
        del self.abind

    modlist = ((),
               ('shift',), 
               ('shift',), 
               ('shift',),
               ('control',),
               ('shift','control'), 
               ('shift','control'), 
               ('shift','control'))

    def event_key( self, ev ):
        if len(ev.keysym) == 1:
            k = ev.keysym.lower()
            if k in allowed_keyevents and isinstance( self.object, controller.Controller ):
                system.global_send_event( self.object, self.pdict.get( 't', None ),
                                          ('key', k, ev.x, self.size[1] - ev.y, self.modlist[ev.state&7]) )

    def event_mousedown( self, ev ):
        system.global_send_event( self.object, self.pdict.get( 't', None ),
                                  ('mousedown', ev.x, self.size[1] - ev.y, self.modlist[ev.state&7]) )
        self.mousebind = (self.s.bind( '<Motion>', self.event_mousemove ),
                          self.s.bind( '<ButtonRelease-1>', self.event_mouseup ))

    def event_mousemove( self, ev ):
        system.global_send_event( self.object, self.pdict.get( 't', None ),
                                  ('mousemove', ev.x, self.size[1] - ev.y, self.modlist[ev.state&7]) )
        
    def event_mouseup( self, ev ):
        system.global_send_event( self.object, self.pdict.get( 't', None ),
                                  ('mouseup', ev.x, self.size[1] - ev.y, self.modlist[ev.state&7]) )
        self.s.unbind( '<Motion>', self.mousebind[0] )
        self.s.unbind( '<ButtonRelease-1>', self.mousebind[1] )
        del self.mousebind

    def build_animation_controls( self, index, object, pdict ):
        self.object = object
        self.index = index
        self.pdict = { 't' : 0.0 }

        v = Tk.DoubleVar()
        w = Tk.Scale( self.controls, {'from' : 0.0, 'to' : object.length},
                      res = 0.01, orient = 'h', variable = v, command = self.anim_eval )
        v.set( 0.0 )
        w.grid( row = 0, column = 0, sticky = Tk.EW, padx = 5 )
        self.wcontrols = [w]
        self.tkvars = v

        w = Tk.Frame( self.controls )
        w.grid( row = 1, column = 0, sticky = Tk.EW, padx = 5 )
        self.wcontrols.append( w )

        b = Tk.Button( w, text = 'play', command = self.start_play )
        b.pack( side = 'left' )
        b = Tk.Button( w, text = 'stop', command = self.stop_play )
        b.pack( side = 'left' )
        self.entry = e = Tk.Entry( w, width = 10 )
        e.bind( '<Key-Return>', self.set_animpos )
        e.pack( side = 'left' )
        b = Tk.Button( w, text = 'sample', command = self.regular_sampling )
        b.pack( side = 'left' )
        b = Tk.Button( w, text = 'div sample', command = self.regular_sampling_div )
        b.pack( side = 'left' )

        self.controls.grid_columnconfigure( 0, weight = 1 )
        self.controls.grid_columnconfigure( 1, weight = 0 )

        self.abind = self.root.bind( '<Key-space>', lambda *x: self.start_play( advance = 1 ) )

    def regular_sampling( self ):
        try:
            v = float( self.entry.get() )
        except ValueError:
            return
        t = 0.0
        while t <= self.object.length:
            self.tkvars.set( t )
            self.anim_eval()
            self.save_screen()
            self.s.update()
            print '  saved time', t
            t += v
        self.s.focus()

    def regular_sampling_div( self ):
        try:
            c = int( self.entry.get() )
        except ValueError:
            return
        for i in range(c):
            t = i * self.object.length / (c-1.0)
            self.tkvars.set( t )
            self.anim_eval()
            self.save_screen()
            self.s.update()
            print '  saved time', t
        self.s.focus()

    def set_animpos( self, ev ):
        try:
            v = float( self.entry.get() )
        except ValueError:
            return
        self.tkvars.set( v )
        self.anim_eval()
        self.s.focus()

    def destroy_animation_controls( self ):
        self.stop_play()
        for i in self.wcontrols:
            i.destroy()
        del self.tkvars
        del self.wcontrols

        self.root.unbind( '<Key-Space>', self.abind )
        del self.abind

    def start_play( self, advance = 0 ):
        self.start_time = time.time()
        self.start_t = self.pdict['t']
        if hasattr( self.object, 'length' ):
            if self.start_t >= self.object.length - 0.1:
                if advance and self.index < len(self.objectlist)-1 and isinstance( self.objectlist[self.index+1][0], anim.Animation ):
                    self.set_object( self.index+1 )
                self.start_t = 0.0
        self.run_mode = 0
        self.frame_count = 0
        if self.idle_id is None:
            self.idle_id = self.s.after( self.timeout_interval, self.next_frame )

    def stop_play( self ):
        if self.idle_id is not None:
            self.s.after_cancel( self.idle_id )
            self.idle_id = None
            self.run_mode = None

    def reset_play( self ):
        self.stop_play()
        self.object = self.objectclass()
        self.pdict = { 't' : 0.0 }
        self.start_play()

    def next_frame( self ):
        now = time.time()
        if self.run_mode == 0:
            t = now - self.start_time + self.start_t
            if hasattr( self.object, 'length') and t > self.object.length:
                t = self.object.length
            if self.tkvars is not None:
                self.tkvars.set( t )

        self.frame_count += 1
        self.pdict['t'] = t
        self.marklist = None
        self.s.redraw()

        if (self.run_mode == 0 and hasattr( self.object, 'length' ) and t >= self.object.length):
            self.idle_id = None

            try:
                print '%r complete (%d frames / %.2f sec = %.3f fps)' % \
                      (self.object.name, self.frame_count, now - self.start_time,
                       self.frame_count / (now - self.start_time))
            except ZeroDivisionError:
                print 'zero length animation'
        else:
            self.idle_id = self.s.after( self.timeout_interval, self.next_frame )
    

    def build_diagram_controls( self, index, object, pdict ):
        params = system.get_object_params( object )
        ranges = system.get_object_param_ranges( object )

        oldpdict = self.pdict
        self.pdict = {}

        self.wcontrols = []
        self.wlabels = []
        self.tkvars = {}

        j = 0
        for name, ptype, value in params:
            if name in pdict:
                self.pdict[name] = pdict[name]
            elif name in oldpdict:
                self.pdict[name] = oldpdict[name]
            else:
                self.pdict[name] = value

            if ptype == parameters.SCALAR:
                wl = Tk.Label( self.controls, text = name, font = self.labelfont )
                wl.grid( row = j, column = 0, sticky = Tk.E )
                self.wlabels.append( wl )

                v = Tk.DoubleVar()
                w = Tk.Scale( self.controls, {'from' : ranges[name][0], 'to' : ranges[name][1]}, width = 5,
                              res = math.fabs( ranges[name][1] - ranges[name][0] ) * 0.001, orient = 'h',
                              variable = v, command = self.diagram_eval )
                v.set( self.pdict[name] )
                w.grid( row = j, column = 1, sticky = Tk.EW )
                j += 1
                self.wcontrols.append( w )
                self.tkvars[name] = v

            elif ptype == parameters.INTEGER:
                wl = Tk.Label( self.controls, text = name, font = self.labelfont )
                wl.grid( row = j, column = 0, sticky = Tk.E )
                self.wlabels.append( wl )

                v = Tk.IntVar()
                w = Tk.Scale( self.controls, {'from' : ranges[name][0], 'to' : ranges[name][1]}, width = 5,
                              res = 1, orient = 'h', variable = v, command = self.diagram_eval )
                v.set( self.pdict[name] )
                w.grid( row = j, column = 1, sticky = Tk.EW )
                j += 1
                self.wcontrols.append( w )
                self.tkvars[name] = v

            elif ptype == parameters.BOOLEAN:
                wl = Tk.Label( self.controls, text = name, font = self.labelfont )
                wl.grid( row = j, column = 0, sticky = Tk.E )
                self.wlabels.append( wl )

                v = Tk.BooleanVar()
                w = Tk.Checkbutton( self.controls, variable = v, command = self.diagram_eval )
                v.set( self.pdict[name] )
                w.grid( row = j, column = 1, sticky = Tk.W )
                j += 1
                self.wcontrols.append( w )
                self.tkvars[name] = v

            elif ptype == parameters.STRING:
                wl = Tk.Label( self.controls, text = name, font = self.labelfont )
                wl.grid( row = j, column = 0, sticky = Tk.E )
                self.wlabels.append( wl )

                v = Tk.StringVar()
                w = Tk.Entry( self.controls, width = 40, textvariable = v )
                w.bind( '<Key-Return>', self.diagram_eval )
                v.set( self.pdict[name] )
                w.grid( row = j, column = 1, sticky = Tk.W )
                j += 1
                self.wcontrols.append( w )
                self.tkvars[name] = v

            elif ptype == parameters.COLOR:
                wl = Tk.Label( self.controls, text = name, font = self.labelfont )
                wl.grid( row = j, column = 0, sticky = Tk.E )
                self.wlabels.append( wl )

                c = self.pdict[name]
                vr = Tk.DoubleVar()
                vr.set( c[0] )
                vg = Tk.DoubleVar()
                vg.set( c[1] )
                vb = Tk.DoubleVar()
                vb.set( c[2] )
                va = Tk.DoubleVar()
                va.set( c[3] )
                
                w = Tk.Frame( self.controls )
                wr = Tk.Scale( w, {'from' : 0.0, 'to' : 1.0}, width = 5,
                               res = 0.01, orient = 'h', variable = vr, command = self.diagram_eval )
                wg = Tk.Scale( w, {'from' : 0.0, 'to' : 1.0}, width = 5,
                               res = 0.01, orient = 'h', variable = vg, command = self.diagram_eval )
                wb = Tk.Scale( w, {'from' : 0.0, 'to' : 1.0}, width = 5,
                               res = 0.01, orient = 'h', variable = vb, command = self.diagram_eval )
                wa = Tk.Scale( w, {'from' : 0.0, 'to' : 1.0}, width = 5,
                               res = 0.01, orient = 'h', variable = va, command = self.diagram_eval )
                wr.pack( side = 'left', fill = 'x', expand = 1 )
                wg.pack( side = 'left', fill = 'x', expand = 1 )
                wb.pack( side = 'left', fill = 'x', expand = 1 )
                wa.pack( side = 'left', fill = 'x', expand = 1 )
                
                w.grid( row = j, column = 1, sticky = Tk.EW )
                j += 1
                self.wcontrols.append( w )
                self.tkvars[name] = (vr, vg, vb, va)
                
                
                

        self.controls.grid_columnconfigure( 1, weight = 1 )
        self.controls.grid_columnconfigure( 0, weight = 0 )

        self.object = object
        self.index = index

    def destroy_diagram_controls( self ):
        for i in self.wcontrols:
            i.destroy()
        for i in self.wlabels:
            i.destroy()
        del self.wcontrols
        del self.wlabels
        del self.tkvars

    def process_objectlist( self, olist ):
        self.objectlist = []
        empty = {}
        self.process_olist_recurse( olist, empty )

    def process_olist_recurse( self, olist, empty ):
        for i in olist:
            
            #if isinstance( i, tuple ) and len(i) == 2:
            #    print i
            #    fn, pd = i
            #    self.objectlist.append( (fn, pd, fn.func_name) )

            if isinstance( i, anim.Animation ):
                self.objectlist.append( (i, empty, i.name) )
            elif isinstance( i, type) and issubclass( i, controller.Controller ):
                self.objectlist.append( (i, empty, i.__name__) )
            elif isinstance( i, tuple ):
                self.objectlist.append( (i, empty, i[2]) )
            elif callable( i ):
                self.objectlist.append( (i, empty, i.func_name) )
            elif isinstance( i, list ):
                self.process_olist_recurse( i, empty )
            else:
                raise ValueError, ("not a drawing object", i)

    def build_object_menu( self ):
        om = self.menubar.component('Object')

        parent = {}
        
        for i in range(len(self.objectlist)):
            o, p, n = self.objectlist[i]
            if isinstance( o, anim.PartialAnimation ):
                a = o.anim
                if a not in parent:
                    name = str(a.name)
                    parent[a] = Tk.Menu( om )
                    om.add_cascade( label = name, menu = parent[a] )
                parent[a].add_command( label = str(n), command = lambda i=i, *x: self.set_object( i ) )
            else:
                om.add_command( label = str(n), command = lambda i=i, *x: self.set_object( i ) )

    def toggle_controls( self, ev ):
        if self.controls_shown:
            self.controls.pack_forget()
            self.root.config( menu = None )
            self.controls_shown = 0
        else:
            self.controls.pack( fill = 'x', padx = 10, pady = 10 )
            self.root.config( menu = self.menubar )
            self.controls_shown = 1
            
        


def test_objects( *diagrams, **keywd ):
    # if caller is not being run as script, return
    import inspect
    try:
        if inspect.stack()[1][0].f_globals['__name__'] != '__main__':
            return
    except (IndexError, KeyError, AttributeError):
        return

    try:
        opts, args = getopt.getopt( sys.argv[1:], 'v:',
                                    ['visual='] )
    except getopt.GetoptError:
        print "didn't understand command-line args"
        sys.exit( 1 )

    visual = os.getenv( 'SLITHY_VISUAL', keywd.get( 'visual', None ) )
                        
    for o, a in opts:
        if o in ('-v', '--visual'):
            visual = a

    screen_size = keywd.get( 'screen_size', (800, 600) )
    clear_color = keywd.get( 'clear_color', colors.white )
    
    tk = Pmw.initialise()
    v = ObjectViewer( tk, diagrams, screen_size, clear_color, visual )
    try:
        tk.mainloop()
    except KeyboardInterrupt:
        sys.exit(1)

