import Tkinter
import sys

class Soggy(Tkinter.Widget):
    """Soggy widget."""
    def __init__(self, master=None, cnf={}, **kw):
        """Construct a Soggy widget with the parent MASTER.

        Valid resource names: width, height, reshape, redraw, init,
        <red, green, blue, alpha, accumred, accumgreen, accumblue,
        accumalpha, depth, stencil>.

        Once the widget is created, reading the values of the options
        listed in <brackets> will return the actual characteristics of
        the visual obtained (not necessarily the values that were
        originally requested.)  Setting these options after the widget
        is created has no effect (other than to overwrite the
        potentially useful values stored in them.)"""
        
        if not master:
            if Tkinter._support_default_root:
                loadee = Tkinter._default_root
            else:
                loadee = None
        else:
            loadee = master
        loadee.tk.call( 'load', '', 'Slsoggy' )
        Tkinter.Widget.__init__(self, master, 'Slsoggy::soggy', cnf, kw)

    def redraw( self, *dummy ):
        self.tk.call( self._w, 'redraw' )
        
    def reshape( self, *dummy ):
        self.tk.call( self._w, 'reshape' )
        
    def init( self, *dummy ):
        self.tk.call( self._w, 'init' )

    def eval( self, cb ):
        if callable(cb):
            cb = self._register( cb )
        return self.tk.call( self._w, 'eval', cb )

    if sys.platform == 'win32':
        def gethwnd( self, *dummy ):
            return self.tk.call( self._w, 'gethwnd' )
            
