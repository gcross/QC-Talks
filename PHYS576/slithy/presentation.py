import Tkinter as Tk
import SlSoggy
import time
import error
import anim
import sys
import search
import system, library, getopt, os
import draw, dobj, objects


#sys.excepthook = error.show_user_error

__all__ = []
all = __all__

timeout_interval = 15
click_interval = 100
movement_threshold = 10
mousehide_delay = 3000
presentation_start = time.time()
quicktime_time = 0.0
quicktime_framerate = 10 # fps

try:
    import dxvideo
    dxvideo.initialize()
    video_available = 1
except ImportError:
    video_available = 0

class PresentationViewer:
    def add_bookmark( self, name ):
        self.bookmarks.append( (str(name), len(self.segments)) )
        
    def play_animations( self, *items, **kw ):
        pause_between = kw.get( 'pause_between', 1 )
        pause_after = kw.get( 'pause_after', 0 )
        for k in kw.iterkeys():
            if k not in ('pause_between', 'pause_after'):
                raise TypeError, "play_animations() got an unexpected keyword argument '%s'" % (k,)
        
        aitems = []
        stack = list(items)
        stack.reverse()
        while stack:
            i = stack.pop()
            if callable( i ):
                try:
                    i = error.call( i )
                except ValueError:
                    continue

            if isinstance( i, anim.Animation ):
                aitems.append( i )
            elif type(i) is tuple or type(i) is list:
                x = list(i)
                x.reverse()
                stack.extend( x )
            else:
                raise ValueError, 'bad argument to play_animations'

        if not aitems:
            return
        
        for i in aitems[:-1]:
            self.add_segment( 'play', i )
            if pause_between:
                self.add_segment( 'pause' )
        self.add_segment( 'play', aitems[-1] )
        if pause_after:
            self.add_segment( 'pause' )

    def add_pause( self ):
        self.add_segment( 'pause' )

    def add_generate( self, info ):
        self.add_segment( 'generate', info )

    def add_segment( self, type, *args ):
        self.segments.append( (type, tuple(self.current_extras), args) )

    def go( self, screen_size = (800,600), root = None, visual = None ):
        if not root:
            root = Tk.Tk()
        self.root = root
        root.wm_title( 'Slithy presentation' )

        root.wm_protocol( 'WM_DELETE_WINDOW', self.quit )

        self.size = None
        self.pause = 0
        self.idle_id = None
        self.segment_num = None
        self.keepalive = 0
        self.clicktimer = None
        self.cursor = 1
        self.grab_filename = None
        self.grab_next = 1
        self.advance_count = 0
        self.object = None

        if self.visual:
            visual = self.visual

        if(self.quicktime):
            if not os.path.exists("quicktime-frames"):
                os.mkdir("quicktime-frames")

        try:
            if visual:
                d = system.parse_visual_arg( visual )
            else:
                d = {}

            self.s = SlSoggy.Soggy( root, width = screen_size[0], height = screen_size[1],
                                    init = dobj.init, reshape = self.reshape, redraw = self.redraw,
                                    **d )
        except (Tk.TclError, ValueError), msg:
            print msg
            sys.exit(1)

        self.show_soggy_visual()

        library._reader.set_doquery( self.eval_query )
        
        self.s.pack( expand = 1, fill = 'both' )
        system.globals.soggy = self.s

        self.create_video_objects()

        root.bind( '<Motion>', self.mouse_motion )
        self.cursortimer = self.s.after( mousehide_delay, self.cursor_expire )
        
        root.bind( '<ButtonPress-1>', self.mouse_down )
        root.bind( '<ButtonPress-3>', self.jump_menu )
        root.bind( '<Key>', self.handle_key )

        self.jmenu = Tk.Menu( self.s, tearoff = 0 )
        for name, pos in self.bookmarks:
            self.jmenu.add_command( label=name, command=lambda j=pos: self.jump_to_segment(j) )

        if self.fullscreen:
            self.fullscreen = 0
            self.toggle_fullscreen()

#         j = 0
#         for i in self.segments:
#             print j, i
#             j += 1

        self.begin_segment( 0 )

        root.mainloop()

    def show_soggy_visual( self ):
        r = self.s.cget( 'red' )
        g = self.s.cget( 'green' )
        b = self.s.cget( 'blue' )
        a = self.s.cget( 'alpha' )
        depth = self.s.cget( 'depth' )
        stencil = self.s.cget( 'stencil' )

        print '     visual : %s/%s/%s/%s/%s/%s' % (r, g, b, a, depth, stencil)

    def create_video_objects( self ):
        print 'creating %d video objects' % (len(objects.Video.all_videos),)
        for i in objects.Video.all_videos:
            i.create_instance()

    def cursor_expire( self ):
        self.cursor = 0
        self.s.config( showcursor = 0 )

    def mouse_motion( self, ev ):
        if self.cursor == 0:
            self.cursor = 1
            self.s.config( showcursor = 1 )
            if self.cursortimer:
                self.s.after_cancel( self.cursortimer )
                self.cursortimer = self.s.after( mousehide_delay, self.cursor_expire )
        
    def reshape( self, w, h ):
        self.size = (int(w), int(h))
        system.globals.size = self.size
        self.aspect = float(w) / float(h)
        dobj.reshape( *self.size )

    def redraw( self ):
        if self.size is None or self.object is None:
            dobj.blank()
            return

        library._reader.mode = 1
        
        draw._save_marked_cameras( 0 )
        dobj.startdraw( None )
        draw._frame()
        system.event_receivers.reset()
        self.keepalive = system.global_draw_object( self.object, self.pdict, self.aspect, 1.0 )
        if self.grab_filename:
            dobj.enddraw( self.grab_filename )
            self.grab_filename = None
        else:
            dobj.enddraw()

        library._reader.mode = 0
        
    def eval_query( self, xy ):
        self.s.eval( lambda: self.query_redraw( xy ) )
        return self.lastqueryresult

    def query_redraw( self, xy ):
        library._reader.mode = 1
        
        draw._save_marked_cameras( 0 )
        dobj.startdraw( library.black, xy )
        draw._frame()
        system.event_receivers.reset()
        system.global_draw_object( self.object, self.pdict, self.aspect, 1.0 )
        self.lastqueryresult = dobj.enddraw()
        
        library._reader.mode = 0

    def jump_to_segment( self, num ):
        try:
            seg = self.segments[num]
        except IndexError:
            return
        
        self.begin_segment( num )

    def begin_segment( self, num ):
        self.segment_num = num
        
        if self.idle_id:
            self.s.after_cancel( self.idle_id )
            self.idle_id = None
            
        try:
            seg = self.segments[num]
        except IndexError:
            if self.auto and not self.quicktime:
                # loop if we're on auto-play, unless we're writing frames
                # for a movie.
                seg = self.segments[0]
                self.segment_num = 0
            else:
                if num < 0:
                    self.segment_num = 0
                else:
                    self.segment_num = len(self.segments) - 1
                self.pause_segment()
                return

        segtype, ex, args = seg
        if segtype == 'pause':
            for i in range(self.segment_num-1, -1, -1):
                if self.segments[i][0] == 'play':
                    self.play_segment_end( *self.segments[i][-1] )
                    break
            else:
                print 'first segment is pause'

        self.pause = 0
        self.dispatch[segtype]( *((self,) + args) )

    def pause_segment( self ):
        self.pause = 1
        dobj.notify( 1 )
        if self.keepalive:
            self.idle_id = self.s.after( timeout_interval, self.pause_redraw )
        if self.auto:
            self.s.after( 500, self.simulate_spacebar )

    def pause_redraw( self ):
        #self.pdict['t'] = time.time() - self.start_time
        self.s.redraw()
        self.s.update_idletasks()
        if self.pause and self.keepalive:
            self.idle_id = self.s.after( timeout_interval, self.pause_redraw )

    def advance( self ):
        if self.pause:
            self.advance_count += 1
            if self.idle_id:
                self.s.after_cancel( self.idle_id )
                self.idle_id = None
            self.pause = 0
            self.begin_segment( self.segment_num + 1 )

    def play_segment( self, anim ):
        dobj.notify( 0 )
        self.object = anim
        self.pdict = { 't' : 0.0 }
        if(self.quicktime):
            self.start_quicktime_time = quicktime_time
        else:
            self.start_time = time.time() 
        self.start_t = 0.0
        self.frame_count = 0
        self.idle_id = self.s.after( self.timeout_interval, self.next_frame )
        self.s.redraw()

    def play_segment_end( self, anim ):
        self.object = anim
        self.pdict = { 't' : anim.length }
        self.s.redraw()
        self.s.update()
        if self.idle_id:
            self.s.after_cancel( self.idle_id )
            self.idle_id = None

    def generate_segment( self, key ):
        class Blank: pass
        ev = Blank()
        ev.keysym = key
        ev.state = 0
        ev.keycode = 1
        ev.x = 0
        ev.y = 0
        print 'simulating key "%s"' % (key,)
        self.handle_key( ev )
        self.begin_segment( self.segment_num + 1 )
        
    
    def next_frame( self ):
        global quicktime_time
        if(self.quicktime):
            quicktime_time += 1.0/quicktime_framerate
            t = quicktime_time - self.start_quicktime_time
        else:
            now = time.time() 
            # start_t always seems to be zero, start time is when the
            # segment began.  now is.
            t = now - self.start_time + self.start_t

        if hasattr( self.object, 'length') and t > self.object.length:
            t = self.object.length

        self.frame_count += 1
        self.pdict['t'] = t
        if(self.quicktime):
            self.s.redraw()
            self.grab_screen()
        else:
            self.s.redraw()

        if t >= self.object.length:
            self.idle_id = None
            self.begin_segment( self.segment_num + 1 )
        else:
            self.idle_id = self.s.after( self.timeout_interval, self.next_frame )

    dispatch = { 'pause' : pause_segment,
                 'play' : play_segment,
                 'generate' : generate_segment }

    def jump_to_start( self ):
        self.begin_segment( 0 )

    def jump_to_end( self ):
        self.begin_segment( len(self.segments)-1 )

    def scan_backwards( self ):
        n = self.segment_num
        if n is None:
            return

        for i in xrange(n-1,0,-1):
            if self.segments[i][0] == 'pause':
                break
        else:
            i = 0

        self.begin_segment( i )

    def scan_forwards( self ):
        n = self.segment_num
        if n is None:
            return
        for i in xrange(n+1,len(self.segments)):
            if self.segments[i][0] == 'pause':
                self.begin_segment( i )
                break
        else:
            self.jump_to_end()

            

    def jump_menu( self, ev ):
        self.jmenu.tk_popup( ev.x_root, ev.y_root )

    def quit( self ):
        self.root.quit()
        if video_available:
            dxvideo.uninitialize()

    def toggle_fullscreen( self ):
        if self.fullscreen:
            self.fullscreen = 0
            self.root.wm_state( 'normal' )
            self.root.wm_overrideredirect( 0 )
            self.fodder.destroy()
            self.root.tkraise()
            self.s.focus()
        else:
            self.fullscreen = 1
            self.fodder = Tk.Toplevel()
            self.fodder.wm_iconify()
            self.fodder.bind( '<FocusIn>', self.fodder_set_focus )
            if sys.platform == 'win32':
                self.root.wm_state( 'zoomed' )
                self.root.wm_overrideredirect( 1 )
                self.root.update()

    def fodder_set_focus( self, ev ):
        self.root.tkraise()
        self.s.focus()

    def grab_screen( self ):
        global presentation_start, quicktime_time
        if(self.quicktime):
            self.grab_filename = 'quicktime-frames/slithy%03d-%04d-%5.3f.png' % (self.advance_count,
                                                                self.grab_next,
                                                                quicktime_time)
            self.grab_next += 1
            self.s.redraw()
        else:
            self.grab_filename = 'slithy%04d-%5.3f.png' % (self.grab_next,
                                                           time.time() -
                                                           presentation_start)
            self.grab_next += 1
            self.s.redraw()
            
    key_dispatch = { 'escape' : quit,
                     'q' : quit,
                     'space' : advance,
                     'tab' : toggle_fullscreen,
                     'less' : scan_backwards,
                     'greater' : scan_forwards,
                     'comma' : scan_backwards,
                     'period' : scan_forwards,
                     'equal' : grab_screen,
                     ('prior',('control',)) : jump_to_start,
                     ('next',('control',)) : jump_to_end,
                     }
    key_modifiers = ((),
                     ('shift',), 
                     ('shift',), 
                     ('shift',),
                     ('control',),
                     ('shift','control'), 
                     ('shift','control'), 
                     ('shift','control'))
                     
    def handle_key( self, ev ):
#          if ev.keycode == 0:
#              for item in self.enabled_extras:
#                  item.handle_special()
#              return
        
        key = ev.keysym.lower()
        mod = self.key_modifiers[ev.state & 7]

        if (key,mod) in self.key_dispatch:
            self.key_dispatch[(key,mod)]( self )
        elif key in self.key_dispatch:
            self.key_dispatch[key]( self )
        else:
            try:
                t = self.pdict['t']
            except (AttributeError, KeyError):
                return
            if self.size is None:
                system.event_receivers.send_event( self.pdict['t'], ('key', key, -1, -1, mod) )
            else:
                system.event_receivers.send_event( self.pdict['t'], ('key', key, ev.x, self.size[1] - ev.y, mod) )

    def handle_mouse( self, evtype, ev ):
        pass
        #x, y = stage.unproject( self.size, self.bgcamera, ev.x, ev.y )
#          redraw = 0
#          for item in self.enabled_extras:
#              redraw = item.handle_mouse( evtype, x, y ) or redraw
#          if redraw:
#              self.s.redraw()


    def simulate_spacebar( self ):
        class Blank: pass
        ev = Blank()
        ev.keysym = 'space'
        ev.state = 0
        ev.keycode = 1
        ev.x = 0
        ev.y = 0
        self.handle_key( ev )

    def mouse_down( self, ev ):
        system.event_receivers.send_event( self.pdict['t'], ('mousedown', ev.x, self.size[1] - ev.y, self.key_modifiers[ev.state & 7]))
        self.mouseinfo = (self.s.bind( '<Motion>', self.mouse_drag ),
                          self.s.bind( '<ButtonRelease-1>', self.mouse_up ))

    def mouse_drag( self, ev ):
        system.event_receivers.send_event( self.pdict['t'], ('mousemove', ev.x, self.size[1] - ev.y, self.key_modifiers[ev.state & 7]))

    def mouse_up( self, ev ):
        system.event_receivers.send_event( self.pdict['t'], ('mouseup', ev.x, self.size[1] - ev.y, self.key_modifiers[ev.state & 7]))
        self.s.unbind( '<Motion>', self.mouseinfo[0] )
        self.s.unbind( '<ButtonRelease-1>', self.mouseinfo[1] )
        del self.mouseinfo

    def __init__( self, start_fullscreen, start_safe, auto, visual, quicktime ):
        self.timeout_interval = timeout_interval
        self.fullscreen = start_fullscreen
        self.segments = []
        self.bookmarks = []
        self.current_background = None
        self.current_extras = []
        if start_safe:
            del self.key_dispatch['escape']
        self.auto = auto
        self.visual = visual
        self.quicktime = quicktime


def usage():
    print 'usage: %s [options]' % (sys.argv[0],)
    print
    print '  -?, --help            show this message and exit'
    print '  -f, --fullscreen      start in fullscreen mode'
    print '  -s, --safe            disable Esc key'
    print '  -a, --auto            auto-advance through presentation'
    print '  -q, --quicktime       output frames for quicktime movie generation (mac)'
    print 
    print '  -v, --visual=VIS      request VIS visual'
    

try:
    opts, args = getopt.getopt( sys.argv[1:], '?v:fsaq',
                                ['help', 'visual=', 'fullscreen', 'safe', 'auto', 'quicktime' ] )
except getopt.GetoptError:
    usage()
    sys.exit( 1 )

fullscreen = 0
safe = 0
auto = 0
quicktime = False
visual = os.getenv( 'SLITHY_VISUAL', None )

for o, a in opts:
    if o in ('-?', '--help'):
        usage()
        sys.exit(0)
    elif o in ('-v', '--visual'):
        visual = a
    elif o in ('-f', '--fullscreen'):
        fullscreen = 1
    elif o in ('-s', '--safe'):
        safe = 1
    elif o in ('-a', '--auto'):
        auto = 1
    elif o in ('-q', '--quicktime'):
        quicktime = True 



curr = PresentationViewer( fullscreen, safe, auto, visual, quicktime )


bookmark = curr.add_bookmark
play = curr.play_animations
pause = curr.add_pause
generate_key = curr.add_generate
run_presentation = curr.go
this = curr

all.append( 'this' )
all.append( 'bookmark' )
all.append( 'play' )
all.append( 'pause' )
all.append( 'generate_key' )
all.append( 'run_presentation' )

#  from search import imagepath
#  all.append( 'imagepath' )

#  import diaimage
#  load_image = diaimage.get_image
#  all.append( 'load_image' )
 
