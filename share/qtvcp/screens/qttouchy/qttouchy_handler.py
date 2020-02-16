############################
# **** IMPORT SECTION **** #
############################
import sys
import os
import linuxcnc
import hal

from PyQt5 import QtCore, QtWidgets, QtGui

from qtvcp.widgets.mdi_line import MDILine as MDI_WIDGET
from qtvcp.widgets.gcode_editor import GcodeEditor as GCODE
from qtvcp.widgets.richfontselector import RichTextEditorDialog
from qtvcp.widgets.stylesheeteditor import  StyleSheetEditor as SSE
from qtvcp.lib.keybindings import Keylookup
from qtvcp.core import Status, Action, Info

# Set up logging
from qtvcp import logger
LOG = logger.getLogger(__name__)

# Set the log level for this module
#LOG.setLevel(logger.INFO) # One of DEBUG, INFO, WARNING, ERROR, CRITICAL

###########################################
# **** instantiate libraries section **** #
###########################################

KEYBIND = Keylookup()
STATUS = Status()
ACTION = Action()
INFO = Info()

###################################
# **** HANDLER CLASS SECTION **** #
###################################

class HandlerClass:

    ########################
    # **** INITIALIZE **** #
    ########################
    # widgets allows access to  widgets from the qtvcp files
    # at this point the widgets and hal pins are not instantiated
    def __init__(self, halcomp,widgets,paths):
        self.hal = halcomp
        self.w = widgets
        self.PATHS = paths
        self.current_mode = (None,None)
        self._last_count = 0
        self.rted = RichTextEditorDialog()
        self.STYLEEDITOR = SSE(widgets,paths)

    ##########################################
    # Special Functions called from QTSCREEN
    ##########################################

    # at this point:
    # the widgets are instantiated.
    # the HAL pins are built but HAL is not set ready
    def initialized__(self):
        KEYBIND.add_call('Key_F12','on_keycall_F12')

        self.pin_mpg_in = self.hal.newpin('mpg-in',hal.HAL_S32, hal.HAL_IN)
        self.pin_mpg_in.value_changed.connect(lambda s: self.external_mpg(s))

        self.pin_cycle_start_in = self.hal.newpin('cycle-start-in',hal.HAL_BIT, hal.HAL_IN)
        self.pin_cycle_start_in.value_changed.connect(lambda s: self.cycleStart(s))

        self.pin_abort = self.hal.newpin('abort',hal.HAL_BIT, hal.HAL_IN)
        self.pin_abort.value_changed.connect(lambda s: self.abort(s))

        self.wheel_x = self.hal.newpin('jog.wheel.x',hal.HAL_BIT, hal.HAL_OUT)
        self.wheel_y = self.hal.newpin('jog.wheel.y',hal.HAL_BIT, hal.HAL_OUT)
        self.wheel_z = self.hal.newpin('jog.wheel.z',hal.HAL_BIT, hal.HAL_OUT)

        self.jog_increment = self.hal.newpin('jog.wheel.incement',hal.HAL_FLOAT, hal.HAL_OUT)

        STATUS.connect('feed-override-changed', lambda w, data: self.w.pushbutton_fo.setText('FO {0:.0f}%'.format(data)))
        STATUS.connect('rapid-override-changed', lambda w, data: self.w.pushbutton_ro.setText('RO {0:.0f}%'.format(data)))
        STATUS.connect('spindle-override-changed', lambda w, data: self.w.pushbutton_so.setText('SO {0:.0f}%'.format(data)))
        STATUS.connect('jogincrement-changed', lambda w, incr,label:self.updateIncrementPin(incr))

        if self.w.PREFS_:
            try:
                value = self.w.PREFS_.getpref('DRO_Font', '', str, 'CUSTOM_FORM_ENTRIES')
                if value != '':
                    font = QtGui.QFont()
                    font.fromString(value)
                    self.setDROFont(font)
            except:
                pass
            try:
                value = self.w.PREFS_.getpref('DRO_Color', '', str, 'CUSTOM_FORM_ENTRIES')
                if value != '':
                    self.setDROColor(value)
            except:
                pass


    def processed_key_event__(self,receiver,event,is_pressed,key,code,shift,cntrl):
        # when typing in MDI, we don't want keybinding to call functions
        # so we catch and process the events directly.
        # We do want ESC, F1 and F2 to call keybinding functions though
        if code not in(QtCore.Qt.Key_Escape,QtCore.Qt.Key_F1 ,QtCore.Qt.Key_F2,
                    QtCore.Qt.Key_F3,QtCore.Qt.Key_F5,QtCore.Qt.Key_F5):

            # search for the top widget of whatever widget received the event
            # then check if it's one we want the keypress events to go to
            flag = False
            receiver2 = receiver
            while receiver2 is not None and not flag:
                if isinstance(receiver2, QtWidgets.QDialog):
                    flag = True
                    break
                if isinstance(receiver2, MDI_WIDGET):
                    flag = True
                    break
                if isinstance(receiver2, GCODE):
                    flag = True
                    break
                receiver2 = receiver2.parent()

            if flag:
                if isinstance(receiver2, GCODE):
                    # if in manual do our keybindings - otherwise
                    # send events to gcode widget
                    if STATUS.is_man_mode() == False:
                        if is_pressed:
                            receiver.keyPressEvent(event)
                            event.accept()
                        return True
                elif is_pressed:
                    receiver.keyPressEvent(event)
                    event.accept()
                    return True
                else:
                    event.accept()
                    return True

        # ok if we got here then try keybindings
        try:
            return KEYBIND.call(self,event,is_pressed,shift,cntrl)
        except NameError as e:
            LOG.debug('Exception in KEYBINDING: {}'.format (e))
        except Exception as e:
            LOG.debug('Exception in KEYBINDING:', exc_info=e)
            print 'Error in, or no function for: %s in handler file for-%s'%(KEYBIND.convert(event),key)
            return False

    ########################
    # callbacks from STATUS #
    ########################

    def updateIncrementPin(self, incr):
        self.jog_increment.set(incr)

    #######################
    # callbacks from form #
    #######################

    def updateJogState(self):
        state = self.w.pushbutton_jog.isChecked()
        if state:
            ACTION.SET_MANUAL_MODE()
        selected = STATUS.get_selected_axis()
        for temp in INFO.AVAILABLE_AXES:
            if selected == temp:
                self['wheel_{}'.format(temp.lower())].set(state)
            else:
                self['wheel_{}'.format(temp.lower())].set(False)

    def colorDialog(self):
        color = QtWidgets.QColorDialog.getColor()
        self.setDROColor(color.name())

    def fontDialog(self):
        font, ok = QtWidgets.QFontDialog.getFont(self.w.dro_label_1.font())
        if ok:
            self.setDROFont(font)

    def togglePointer(self, data):
        if data:
            QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.BlankCursor)
        else:
            QtWidgets.QApplication.restoreOverrideCursor()

    #####################
    # general functions #
    #####################

    def setDROFont(self, font):
        for i in range(1,10):
            self.w['dro_label_{}'.format(i)].setFont(font)

    def setDROColor(self, color):
        for i in range(1,10):
            self.w['dro_label_{}'.format(i)].setStyleSheet("QWidget { color: %s}" % color)

    def abort(self, state):
        if not state:
            return
        if STATUS.stat.interp_state == linuxcnc.INTERP_IDLE:
            self.w.close()
        else:
            ACTION.ABORT()

    def cycleStart(self, state):
        print state, self.current_mode
        if state:
            tab = self.w.mainTab.currentWidget()
            if  tab in( self.w.tab_auto,  self.w.tab_graphics):
                print 'start cycle!', self.w.gcode_editor.get_line()
                ACTION.RUN(line=0)
            elif tab == self.w.tab_files:
                    print 'load program'
                    self.w.filemanager.load()
            elif tab == self.w.tab_mdi:
                print 'run MDI'
                self.w.mditouchy.run_command()

    # MPG scrolling of program or MDI history
    def external_mpg(self, count):
        diff = count - self._last_count
        if self.w.pushbutton_scroll.isChecked():
            if self.w.mainTab.currentWidget() == self.w.tab_auto:
                self.w.gcode_editor.jump_line(diff)
            elif self.w.mainTab.currentWidget() == self.w.tab_files:
                if diff <0:
                   self.w.filemanager.down()
                else:
                    self.w.filemanager.up()
        elif self.w.pushbutton_fo.isChecked():
            scaled = (STATUS.stat.feedrate * 100 + diff)
            if scaled <0 :scaled = 0
            elif scaled > INFO.MAX_FEED_OVERRIDE:scaled = INFO.MAX_FEED_OVERRIDE
            ACTION.SET_FEED_RATE(scaled)
        elif self.w.pushbutton_ro.isChecked():
            scaled = (STATUS.stat.rapidrate * 100 + diff)
            if scaled <0 :scaled = 0
            elif scaled > 100:scaled = 100
            ACTION.SET_RAPID_RATE(scaled)
        elif self.w.pushbutton_so.isChecked():
            scaled = (STATUS.stat.spindle[0]['override'] * 100 + diff)
            if scaled < INFO.MIN_SPINDLE_OVERRIDE:scaled = INFO.MIN_SPINDLE_OVERRIDE
            elif scaled > INFO.MAX_SPINDLE_OVERRIDE:scaled = INFO.MAX_SPINDLE_OVERRIDE
            ACTION.SET_SPINDLE_RATE(scaled)
        self._last_count = count

    # keyboard jogging from key binding calls
    # double the rate if fast is true 
    def kb_jog(self, state, joint, direction, fast = False, linear = True):
        if not STATUS.is_man_mode() or not STATUS.machine_is_on():
            return
        if linear:
            distance = STATUS.get_jog_increment()
            rate = STATUS.get_jograte()/60
        else:
            distance = STATUS.get_jog_increment_angular()
            rate = STATUS.get_jograte_angular()/60
        if state:
            if fast:
                rate = rate * 2
            ACTION.JOG(joint, direction, rate, distance)
        else:
            ACTION.JOG(joint, 0, 0, 0)

    #####################
    # KEY BINDING CALLS #
    #####################

    # Machine control
    def on_keycall_ESTOP(self,event,state,shift,cntrl):
        if state:
            ACTION.SET_ESTOP_STATE(STATUS.estop_is_clear())
    def on_keycall_POWER(self,event,state,shift,cntrl):
        if state:
            ACTION.SET_MACHINE_STATE(not STATUS.machine_is_on())
    def on_keycall_HOME(self,event,state,shift,cntrl):
        if state:
            if STATUS.is_all_homed():
                ACTION.SET_MACHINE_UNHOMED(-1)
            else:
                ACTION.SET_MACHINE_HOMING(-1)
    def on_keycall_ABORT(self,event,state,shift,cntrl):
        if state:
            if STATUS.stat.interp_state == linuxcnc.INTERP_IDLE:
                self.w.close()
            else:
                self.cmnd.abort()

    # Function keys
    def on_keycall_F12(self,event,state,shift,cntrl):
        if state:
            self.STYLEEDITOR.load_dialog()

    # Linear Jogging
    def on_keycall_XPOS(self,event,state,shift,cntrl):
        self.kb_jog(state, 0, 1, shift)

    def on_keycall_XNEG(self,event,state,shift,cntrl):
        self.kb_jog(state, 0, -1, shift)

    def on_keycall_YPOS(self,event,state,shift,cntrl):
        self.kb_jog(state, 1, 1, shift)

    def on_keycall_YNEG(self,event,state,shift,cntrl):
        self.kb_jog(state, 1, -1, shift)

    def on_keycall_ZPOS(self,event,state,shift,cntrl):
        self.kb_jog(state, 2, 1, shift)

    def on_keycall_ZNEG(self,event,state,shift,cntrl):
        self.kb_jog(state, 2, -1, shift)

    def on_keycall_APOS(self,event,state,shift,cntrl):
        if 'A' in INFO.AVAILABLE_AXES:
            self.kb_jog(state, 3, 1, shift, False)

    def on_keycall_ANEG(self,event,state,shift,cntrl):
        if 'A' in INFO.AVAILABLE_AXES:
            self.kb_jog(state, 3, -1, shift, linear=False)

    ###########################
    # **** closing event **** #
    ###########################
    def closing_cleanup__(self):
        if self.w.PREFS_:
            self.w.PREFS_.putpref('DRO_Font', self.w.dro_label_1.font().toString(), str, 'CUSTOM_FORM_ENTRIES')
            color =  self.w.dro_label_1.palette().color(QtGui.QPalette.Foreground).name()
            self.w.PREFS_.putpref('DRO_Color', color, str, 'CUSTOM_FORM_ENTRIES')

    ##############################
    # required class boiler code #
    ##############################

    def __getitem__(self, item):
        return getattr(self, item)
    def __setitem__(self, item, value):
        return setattr(self, item, value)

################################
# required handler boiler code #
################################

def get_handlers(halcomp,widgets,paths):
     return [HandlerClass(halcomp,widgets,paths)]
