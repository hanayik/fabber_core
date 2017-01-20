import matplotlib
matplotlib.use('Qt4Agg')
import pylab

from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure

from PySide.QtGui import QHBoxLayout

from mvc import View
from imagedata import CH_DATA, CH_FOCUS

class OrthoView(View):
    def __init__(self, frame):
        View.__init__(self, [CH_DATA, CH_FOCUS])
        self.fig = Figure(figsize=(400, 400), facecolor='black')
        self.fig.subplots_adjust(0.01, 0.015, 0.99, 0.99, 0.01, 0.01)
        self.canvas = FigureCanvas(self.fig)
        layout = QHBoxLayout()
        layout.addWidget(self.canvas)
        frame.setLayout(layout)
        self.canvas.mpl_connect("button_press_event", self.onpick)
        
        self.axes = []
        self.axes.append(self.fig.add_subplot(221, axisbg='black'))
        self.axes.append(self.fig.add_subplot(222, axisbg='black'))
        self.axes.append(self.fig.add_subplot(223, axisbg='black'))
        self.tbox = self.fig.add_subplot(224, axisbg='black')
        self.ftext = self.tbox.text(0.2, 0.5, "", color='white')
        self.xhairs, self.yhairs = [0,0,0], [0,0,0]
        for i in range(3):
            self.yhairs[i] = self.axes[i].axvline(0.5, color='white', linestyle='-', zorder=20)
            self.xhairs[i] = self.axes[i].axhline(0.5, color='white', linestyle='-', zorder=20)
            
        self.maxdim = 1
        self.config_axes()

        self.imgs = [{}, {}, {}]

    def onpick(self, event):
        if self.imagedata is None: return
        x, y = int(event.xdata), int(event.ydata)
        if event.inaxes == self.axes[1]:
            self.imagedata.update_focus(yp=x, zp=y)
        if event.inaxes == self.axes[0]:
            self.imagedata.update_focus(xp=x, zp=y)
        if event.inaxes == self.axes[2]:
            self.imagedata.update_focus(xp=x, yp=y)
        
    def config_axes(self):
        for ax in self.axes:
            ax.get_xaxis().set_visible(False)
            ax.get_yaxis().set_visible(False)
            ax.set_xlim(0, self.maxdim)
            ax.set_ylim(0, self.maxdim)
        self.tbox.get_xaxis().set_visible(False)
        self.tbox.get_yaxis().set_visible(False)
        self.tbox.set_xlim(0, 1)
        self.tbox.set_ylim(0, 1)
        
    def set_extents(self):
        self.maxdim = max(self.imagedata.shape[:3])
        if self.maxdim > 0: self.config_axes()
        
    def add_slices(self, key, item, focus):
        if key == "mask":
            zorder = 5
        elif key == "data":
            zorder = 1
        else:
            zorder = 10
            
        slices = []
        slices.append(item.get_slice(focus, "y").T)
        slices.append(item.get_slice(focus, "x").T)
        slices.append(item.get_slice(focus, "z").T)
        for i, ax in enumerate(self.axes):
            if item.visible:
                ox = (self.maxdim - slices[i].shape[0])/2
                ex = ox + slices[i].shape[0]
                oy = (self.maxdim - slices[i].shape[1])/2
                ey = oy + slices[i].shape[1]
                self.imgs[i][key] = ax.imshow(slices[i], interpolation="nearest", vmin=item.min, vmax=item.max, zorder=zorder)                
                self.imgs[i][key].set_alpha(item.alpha)
                self.imgs[i][key].set_cmap(item.cm)

    def do_update(self):
        self.set_extents()
                    
        focus = self.imagedata.focus
        # Get rid of existing data and recreate
        for img in self.imgs:
            for key in img.keys():
                img[key].remove()
                del img[key]
                    
        for key, item in self.imagedata.data.items():
            print("plotting " + key)
            self.add_slices(key, item, focus)
            
        if max(self.imagedata.shape) > 0:
            for i in range(3):
                if i == 0:
                    self.yhairs[i].set_xdata(focus[0])
                    self.xhairs[i].set_ydata(focus[2])
                elif i == 1:
                    self.yhairs[i].set_xdata(focus[1])
                    self.xhairs[i].set_ydata(focus[2])
                else:
                    self.yhairs[i].set_xdata(focus[0])
                    self.xhairs[i].set_ydata(focus[1])

            self.ftext.set_text("X: %i\nY: %i\nZ: %i\nt: %i" % tuple(focus))
        self.canvas.draw_idle()

class FitView(View):
    def __init__(self, frame):
        View.__init__(self, [CH_DATA, CH_FOCUS])
        self.fig = Figure(figsize=(400, 400))
        self.canvas = FigureCanvas(self.fig)
        layout = QHBoxLayout()
        layout.addWidget(self.canvas)
        frame.setLayout(layout)

        self.ax1 = self.fig.add_subplot(111)
        self.lines = {}

    def plot(self, key, item):
        if item.ndims == 4:
            if item.visible:
                ts = item.get_timeseries(self.imagedata.focus)
                if not self.lines.has_key(key):
                    self.lines[key], = self.ax1.plot(ts)
                else:
                    self.lines[key].set_ydata(ts)

    def do_update(self):
        for key in self.lines.keys():
            self.lines[key].remove()
            del self.lines[key]
        
        if max(self.imagedata.shape) > 0:        
            for key, item in self.imagedata.data.items():
                self.plot(key, item)
           
        self.ax1.relim()
        self.ax1.autoscale_view()
        self.canvas.draw_idle()

