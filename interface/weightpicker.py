
import gobject, gtk, gnoetics
import sys, os, threading, random

class WeightPicker(gobject.GObject):

    def __init__(self, weights):
        gobject.GObject.__init__(self)

        self.__d = gtk.Dialog("Weight Your Source Texts")
        self.__weights = weights

        b = self.__d.add_button("Shuffle", gtk.RESPONSE_NONE)
        b.connect("clicked", lambda b: self.__shuffle_weights())
        
        b = self.__d.add_button("Randomize", gtk.RESPONSE_NONE)
        b.connect("clicked", lambda b: self.__randomize_weights())

        b = self.__d.add_button("Reset", gtk.RESPONSE_NONE)
        b.connect("clicked", lambda b: self.__reset_weights())

        self.__d.add_button(gtk.STOCK_OK, gtk.RESPONSE_CLOSE)

        def response_handler_cb(dialog, id):
            if id != gtk.RESPONSE_NONE:
                self.emit("finished")
                self.destroy()
        self.__d.connect("response", response_handler_cb)


        ###
        ### Assemble the widget
        ###

        all_texts = self.__weights.keys()
        all_texts.sort(lambda x, y: cmp(x.get_sort_title(),
                                        y.get_sort_title()))

        table = gtk.Table(len(all_texts), 3, False)

        self.__labels = []
        self.__adjustments = {}

        for i, txt in enumerate(all_texts):
            label = gtk.Label(txt.get_title())
            label.set_alignment(1.0, 0.5)
            table.attach(label, 0, 1, i, i+1,
                         gtk.FILL, 0,
                         4, 3)

            adj = gtk.Adjustment(weights[txt], 0.0, 10, 0.5, 1, 0)
            self.__adjustments[txt] = adj
            
            scale = gtk.HScale(adj)
            scale.set_draw_value(False)
            scale.set_size_request(100, -1)
            table.attach(scale, 1, 2, i, i+1,
                         gtk.EXPAND | gtk.FILL, 0,
                         4, 3)

            wt_label = gtk.Label("")
            wt_label.set_alignment(0.0, 0.5)
            table.attach(wt_label, 2, 3, i, i+1,
                         gtk.FILL, 0,
                         4, 3)

            self.__labels.append((txt, wt_label))
            self.__update_labels()

            def value_changed_cb(adj, txt, picker):
                x = int(adj.get_value()*10+0.5)/10.0
                picker.set_weight(txt, x, update_adjustment=False)
                picker.__update_labels()
            
            adj.connect("value_changed", value_changed_cb, txt, self)

        table.show_all()

        swin = gtk.ScrolledWindow()
        swin.set_policy(gtk.POLICY_NEVER, gtk.POLICY_AUTOMATIC)
        swin.add_with_viewport(table)
        swin.show_all()

        swin.set_size_request(-1, 200)

        self.__d.vbox.pack_start(swin, expand=1, fill=1)

    def __update_labels(self):
        total = 0.0
        for x in self.__weights.values():
            total += x
            
        for txt, label in self.__labels:
            x = self.__weights[txt]
            label.set_text("%.1f  %4.1f%%" % (x, 100*x/total))


    def set_weight(self, txt, wt, update_adjustment=True):
        self.__weights[txt] = wt
        if update_adjustment:
            self.__adjustments[txt].set_value(wt)
        self.emit("changed_weights", txt, float(wt))


    def __shuffle_weights(self):
        keys = self.__weights.keys()
        vals = self.__weights.values()
        random.shuffle(vals)
        for k, v in zip(keys, vals):
            self.set_weight(k, v)
        self.__update_labels()


    def __randomize_weights(self):
        for txt in self.__weights.keys():
            self.set_weight(txt, random.uniform(0, 10))


    def __reset_weights(self):
        for txt in self.__weights.keys():
            self.set_weight(txt, 1.0)

    def destroy(self):
        self.__d.destroy()

    def show_all(self):
        self.__d.show_all()



gobject.type_register(WeightPicker)

gobject.signal_new("changed_weights",
                   WeightPicker,
                   gobject.SIGNAL_RUN_LAST,
                   gobject.TYPE_NONE,
                   (gobject.TYPE_PYOBJECT, gobject.TYPE_DOUBLE))

gobject.signal_new("finished",
                   WeightPicker,
                   gobject.SIGNAL_RUN_LAST,
                   gobject.TYPE_NONE,
                   ())
