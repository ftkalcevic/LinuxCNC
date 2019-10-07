#!/usr/bin/python2.7
#
# Qtvcp Widgets
# Copyright (c) 2017  Chris Morley <chrisinnanaimo@hotmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
#################################################################################

import os

from PyQt5 import QtWidgets, QtCore, QtGui

from qtvcp.widgets.widget_baseclass import _HalWidgetBase
from qtvcp.widgets.entry_widget import TouchInputWidget
from qtvcp.core import Status, Action, Info
from qtvcp import logger

# Instantiate the libraries with global reference
# INFO holds INI file details
# STATUS gives us status messages from linuxcnc
# ACTION gives commands to linuxcnc
# LOG is for running code logging
INFO = Info()
STATUS = Status()
ACTION = Action()
LOG = logger.getLogger(__name__)

# Set the log level for this module
if not INFO.LINUXCNC_IS_RUNNING:
    LOG.setLevel(logger.ERROR) # One of DEBUG, INFO, WARNING, ERROR, CRITICAL

try:
    from PyQt5 import QtSvg
except:
    LOG.critical("Qtvcp error with macro_widget - is package python-pyqt5.qtsvg installed?")

###############################################################
# helper widget for SVG display on Button
#
# We can add a svg image from a specific layer to QPushButton
###############################################################
class CustomButton(QtWidgets.QPushButton):
    def __init__(self, parent=None, path=None, layer=0):
        super(CustomButton, self).__init__(parent)
        if path is None:
            tpath = os.path.expanduser(INFO.MACRO_PATH)
            path = os.path.join(tpath, 'LatheMacro.svg')
        self.r = QtSvg.QSvgRenderer(path)
        self.basename = 'layer'
        self.setLayerNumber(layer)

    def setLayerNumber(self, num):
        temp = '%s%d' % (self.basename, int(num))
        if self.r.elementExists(temp):
            self.Num = int(num)
            self.layer = temp
        else:
            self.Num = 0
            self.layer = 'layer0'
            LOG.error("MacrosTab SVG Button-No Layer Found: {}".format(temp))

    def paintEvent(self, event):
        super(CustomButton, self).paintEvent(event)
        qp = QtGui.QPainter()
        qp.begin(self)
        self.r.render(qp, self.layer)
        qp.end()


####################################################
# helper widget for SVG display
#
# class to display certain layers of an svg file
# instantiate it with layer number or
# set layer number after with setLayerNumbet(int)
####################################################
class CustomSVG(QtSvg.QSvgWidget):
    def __init__(self, parent=None, layer=0):
        super(CustomSVG, self).__init__(parent)
        self.basename = 'layer'
        self.layer = 'layer%d' % layer
        self.num = layer

    def setLayerNumber(self, num):
        temp = '%s%d' % (self.basename, int(num))
        if self.renderer().elementExists(temp):
            self.Num = int(num)
            self.layer = temp
        else:
            LOG.error("MacrosTab SVG-No Layer Found: {}".format(temp))

    def paintEvent(self, event):
        qp = QtGui.QPainter()
        qp.begin(self)
        s = self.renderer()
        s.render(qp, self.layer)
        qp.end()


###############################################################################
# Macro tab widget
#
# macro tab widget parses the subroutine path for /lathe
# It then opens the .ngc files ther eand searches for keynames
# using these key names it puts together a tab widget with svg file pics
# the svg file should be in the same folder
###############################################################################
class MacroTab(QtWidgets.QWidget, _HalWidgetBase):
    def __init__(self, parent=None):
        super(MacroTab, self).__init__(parent)
        try:
            tpath = os.path.expanduser(INFO.MACRO_PATH)
            self.filepath = os.path.join(tpath, '')
        except:
            self.filepath = 'None'

        # id names for what dialog we want launched
        self.load_dialog_code = 'LOAD'
        self.save_dialog_code = 'SAVE'

        self.stack = QtWidgets.QStackedWidget()

        # add some buttons to run,close and menu
        hbox = QtWidgets.QHBoxLayout()
        self.runButton = QtWidgets.QPushButton("Run")
        self.runButton.pressed.connect(self.runChecked)
        self.saveButton = QtWidgets.QPushButton("save")
        self.saveButton.pressed.connect(self.saveChecked)
        self.saveButton.setVisible(False)

        self.loadButton = QtWidgets.QPushButton("load")
        self.loadButton.pressed.connect(self.loadChecked)
        self.loadButton.setVisible(False)
        self.closeButton = QtWidgets.QPushButton("Close")
        self.closeButton.pressed.connect(self.closeChecked)
        self.closeButton.setVisible(False)

        menuButton = QtWidgets.QPushButton("Menu")
        menuButton.pressed.connect(self.menuChecked)
        hbox.addWidget(self.runButton)
        hbox.addWidget(self.loadButton)
        hbox.addWidget(self.saveButton)
        hbox.addWidget(self.closeButton)
        hbox.insertSpacing(1, 20)
        hbox.addWidget(menuButton)
        hbox.addStretch(0)
        vbox = QtWidgets.QVBoxLayout()
        vbox.addWidget(self.stack)
        vbox.addStretch(1)
        vbox.addLayout(hbox)
        # add all that stuff above to me
        self.setLayout(vbox)
        # add everything else
        self.buildStack()

    def _hal_init(self):
        self.runButton.setEnabled(False)
        STATUS.connect('not-all-homed', lambda w, axis: self.runButton.setEnabled(False))
        STATUS.connect('all-homed', lambda w: self.runButton.setEnabled(True))
        STATUS.connect('general',self.returnFromDialog)

    # Build a stack per macro found
    # it finds the icon info from the macro file
    # using the magic comments parsed before this
    # first find macros
    # then build a menu page
    # then build the stack
    # anything goes wrong display an eror page
    def buildStack(self):
        tabName = self._findMacros()
        LOG.debug("Macros Found: {}".format(tabName))
        if tabName is None:
            self._buildErrorTab()
            return
        self._buildMenuPage(tabName)
        # Add pages
        # tabname is a list of found macros
        # these macro names are also used as the base name
        # of a list of required inputs.
        # we add a label and lineedit/radiobutton for each string in each
        # of these arrays
        for i, tName in enumerate(tabName):
            # make a widget that is added to the stack
            w = TouchInputWidget()
            hbox = QtWidgets.QHBoxLayout(w)
            hbox.addStretch(1)
            vbox = QtWidgets.QVBoxLayout()
            w.setObjectName(tName)
            # add labels and edits
            # self[tName][0] is the list of name text and defaults pairs
            for n, name in enumerate(self[tName][0]):
                l = QtWidgets.QLabel(name[0])
                if name[1].lower() in('false', 'true'):
                    self['%s%d' % (tName, n)] = QtWidgets.QRadioButton()
                    if name[1].lower() == 'true':
                        self['%s%d' % (tName, n)].setChecked(True)
                else:
                    self['%s%d' % (tName, n)] = QtWidgets.QLineEdit()
                    self['%s%d' % (tName, n)].keyboard_type = 'numeric'
                    self['%s%d' % (tName, n)].setText(name[1])
                vbox.addWidget(l)
                vbox.addWidget(self['%s%d' % (tName, n)])
            #add the SVG pic layer
            svg_info = self[tName][1]
            #print self.filepath+svg_info[0], svg_info[1]
            svgpath = os.path.join(self.filepath, svg_info[0])
            self['sw%d' % i] = CustomSVG(svgpath,  int(svg_info[1]))
            hbox.addWidget(self['sw%d' % i])
            vbox.addStretch(1)
            hbox.addLayout(vbox)
            # add the widget to the stack
            self.stack.addWidget(w)

    # Menu page has icon buttons to select the macro
    # it finds the icon info from the macro file
    # using the magic comments parsed before this
    def _buildMenuPage(self, tabNames):
        col = row = 0
        w = QtWidgets.QWidget()
        hbox = QtWidgets.QHBoxLayout(w)
        vbox = QtWidgets.QVBoxLayout()
        grid = QtWidgets.QGridLayout()
        grid.setSpacing(10)
        # we grid them in columns of (arbritrarily) 5
        # hopefully we don;t have too many macros...
        for i, tName in enumerate(tabNames):
            svg_name = self[tName][1][0]
            try:
                svg_num = self[tName][1][2]
            except:
                svg_num = self[tName][1][1]
            svgpath = os.path.join(self.filepath, svg_name)
            # label is the only way I have found to make the buttons
            # larger - the label is under the pic - if no errors
            btn = CustomButton('Oops\n', path=svgpath, layer=svg_num)
            btn.clicked.connect(self.menuButtonPress(i))
            btn.setSizePolicy(QtWidgets.QSizePolicy.Preferred,
                              QtWidgets.QSizePolicy.Expanding)
            grid.addWidget(btn, row, col, 1, 1)
            row += 1
            if row > 4:
                row = 0
                col += 1
        vbox.addLayout(grid)
        vbox.addStretch(1)
        hbox.addLayout(vbox)
        hbox.addStretch(1)
        # add the widget to our stack
        self.stack.addWidget(w)

    # make something so the user may have some small clue.
    # probably should do more - subroutines/macros are not user friendly
    def _buildErrorTab(self):
        w = QtWidgets.QWidget()
        vbox = QtWidgets.QVBoxLayout(w)
        vbox.addStretch(1)
        mess = QtWidgets.QLabel('No Usable Macros Found.\nLooked in path specified in INI file under heading [RS274NGC],\nSUBROUTINE_PATH=')
        vbox.addWidget(mess)
        mess = QtWidgets.QLabel(self.filepath)
        vbox.addWidget(mess)
        # add the widget to the stack
        self.stack.addWidget(w)

    # search for special macros that have the magic comments
    # compiles them into a complicated list for each macro
    # self['macroname'] = [ [DEFAULT DATA],[SVG FILE,LAYER,ICON LAYER],{OPTION DICT NAME:OPTION DICT DATA,}]
    # returns a list on the macro names that it finds valid

    def _findMacros(self):
        path = self.filepath
        tName = []
        macros = []
        defaults = []
        svg_data = []
        try:
            # look for NGC macros in path
            for f in os.listdir(path):
                if f.endswith('.ngc'):
                    # open and look at the first three lines
                    # these will gives us the info we need
                    # to create the macros page
                    with open(os.path.join(path, f), 'r') as temp:
                        first_line = temp.readline().strip()
                        second_line = temp.readline().strip()
                        third_line = temp.readline().strip()
                        fourth_line = temp.readline().strip()
                        # check if they have the magic comments
                        if 'MACROCOMMAND' in first_line and \
                                'MACRODEFAULT' in second_line and \
                                'MACROSVG' in third_line:
                            name = os.path.splitext(f)[0]
                            # yes, now keep everything after '='
                            macros = first_line.split('=')[1]
                            defaults = second_line.split('=')[1]
                            svg_data = third_line.split('=')[1]
                            # we use a comma to break up titles
                            m = macros.split(',')
                            d = defaults.split(',')
                            s = svg_data.split(',')
                            # combine titles with defaults in to a list
                            for num, (m_item, d_item) in enumerate(zip(m, d)):
                                #print num,m_item,d_item
                                if num == 0:
                                    temp = []
                                    temp.append((m_item, d_item))
                                    continue
                                temp.append((m_item, d_item))

                            # look for options like save/load buttons
                            # it should be the fourth line in sub program
                            # it's put in a dict to future proof it
                            # and make it easier to parse
                            option_dict={}
                            if 'MACROOPTIONS' in fourth_line:
                                options = fourth_line.split('=')[1]
                                o = options.split(',')
                                for i in(o):
                                    h,g = i.split(':')
                                    option_dict['%s'%h.upper()]=g
                                #print option_dict

                            # add the list then add svg info, then options
                            self[name] = [temp]
                            self[name].append(s)
                            self[name].append(option_dict)

                            #print'group:',name, self[name]
                            # make a list of pages, which is also the macro program name
                            tName.append(name)
        except (TypeError):
            return None
        except OSError as e:
            LOG.debug('Path: {} is not valid.'.format(path))
            return None
        except Exception as e:
            LOG.debug('Exception loading Macros:', exc_info=e)
            return None
        if tName == []:
            return None
        return tName

    # This figures out what macro is showing
    # and builds the macro command from the data
    # then sends it to the controller
    def runMacro(self):
        cmd = ''
        name = str(self.stack.currentWidget().objectName())
        if name == '': return
        macro = name
        #print 'macro', macro
        for num, i in enumerate(self[name][0]):
            # Look for a radio button instance so we can convert to integers
            # other wise we assume text
            if isinstance(self['%s%d' % (name, num)], QtWidgets.QRadioButton):
                data = str(1 * int(self['%s%d' % (name, num)].isChecked()))
            else:
                data = str(self['%s%d' % (name, num)].text())
            if data != '':
                cmd = cmd + '[' + data + '] '
        command = str("O<" + macro + "> call " + cmd)
        LOG.debug("Macro command: {}".format(command))
        ACTION.CALL_MDI(command)

    # This could be 'class patched' to do something else
    # the macro dialog does this
    def runChecked(self):
        self.runMacro()

    # This could be 'class patched' to do something else
    def saveChecked(self):
        self.savePressed()

    # This could be 'class patched' to do something else
    def loadChecked(self):
        self.loadPressed()

    # This could be 'class patched' to do something else
    def closeChecked(self):
        self.stack.setCurrentIndex(0)

    # brings the menu selection page to the front
    def menuChecked(self):
        self.stack.setCurrentIndex(0)
        self.setTitle('Qtvcp Macro Menu')
        self.loadButton.setVisible(False)
        self.saveButton.setVisible(False)

    # This weird code is just so we can get the index
    # number from a menu button press
    # using clicked.connect() apparently doesn't easily
    # add user data
    def menuButtonPress(self, data):
        def calluser():
            self.stack.setCurrentIndex(data+1)
            try:
                name = self.stack.currentWidget().objectName()
                self.setTitle(name)
                # show these buttons if the macro specifies it
                for name in (self[name][2]):
                    if name == 'LOAD':
                        self.loadButton.setVisible(True)
                    if name == 'SAVE':
                        self.saveButton.setVisible(True)
            except: pass

        return calluser

    def loadPressed(self):
        self.getFileName()

    def savePressed(self):
        self.getSaveFileName()

    def openReturn(self, path):
        qssname = path # os.path.join(DIR, BNAME, sheetName)
        file = QtCore.QFile(qssname)
        file.open(QtCore.QFile.ReadOnly)
        num = 0
        name = str(self.stack.currentWidget().objectName())
        while not file.atEnd():
            dataSheet = file.readLine()
            try:
                # Python v2.
                dataSheet = unicode(dataSheet, encoding='utf8')
            except NameError:
                # Python v3.
                dataSheet = str(dataSheet, encoding='utf8')
            h,g = dataSheet.split(',')

            # set widgets to data:
            # Look for a radio button instance so we can convert to integers
            # other wise we assume text
            if isinstance(self['%s%d' % (name, num)], QtWidgets.QRadioButton):
                #print self['%s%d' % (name, num)], h
                self['%s%d' % (name, num)].setChecked(bool(h))
            else:
                #print self['%s%d' % (name, num)], h
                self['%s%d' % (name, num)].setText(str(h))
            num =+1

    # save the current screen data to file picked by the user.
    # it's a plain text file
    def saveReturn(self, path):
        name = str(self.stack.currentWidget().objectName())
        if name == '': return
        file = QtCore.QFile(path)
        if file.open(QtCore.QFile.WriteOnly):
            for num, i in enumerate(self[name][0]):
                widgetname = '%s%d' % (name, num)
                # Look for a radio button instance so we can convert to bool
                # other wise we assume text
                if isinstance(self[widgetname], QtWidgets.QRadioButton):
                    data = str(1 * int(self[widgetname].isChecked()))
                else:
                    data = str(self[widgetname].text())
                line =  '%s,    %s\n'%( str(data),i[0])
                QtCore.QTextStream(file) << line
        else:
            QMessageBox.information(self, "Unable to open file",
                    file.errorString())

    # we do this instead of directly so the dialog version's title changes
    # when it's overriden
    def setTitle(self, string):
        self.setWindowTitle(string)

    # request the system to pop a load path picker dialog
    # do this so the system is consistant and things like dialog
    # placement are done.
    def getFileName(self):
        mess = {'NAME':self.load_dialog_code,'ID':'%s__' % self.objectName(),
            'TITLE':'Load Macro',
            'FILENAME':'%s_data.txt' % str(self.stack.currentWidget().objectName()),
            'EXTENTIONS':'Text Files (*.txt);;ALL File (*.*)'
            }
        STATUS.emit('dialog-request', mess)

    # request the system to pop a save path picker dialog
    # do this so the system is consistant and things like dialog
    # placement are done.
    def getSaveFileName(self):
        mess = {'NAME':self.save_dialog_code,'ID':'%s__' % self.objectName(),
            'TITLE':'Save Macro', 'FILENAME':'%s_data.txt' % str(self.stack.currentWidget().objectName())}
        STATUS.emit('dialog-request', mess)

    # process the STATUS return message from dialogs
    def returnFromDialog(self, w, message):
        if message.get('NAME') == self.load_dialog_code:
            path = message.get('RETURN')
            code = bool(message.get('ID') == '%s__'% self.objectName())
            if path and code:
                self.openReturn(path)
        elif message.get('NAME') == self.save_dialog_code:
            path = message.get('RETURN')
            code = bool(message.get('ID') == '%s__'% self.objectName())
            if path and code:
                self.saveReturn(path)

    # usual boiler code
    # (used so we can use code such as self[SomeDataName]
    def __getitem__(self, item):
        return getattr(self, item)
    def __setitem__(self, item, value):
        return setattr(self, item, value)

if __name__ == "__main__":
    # STATUS may cause seg fault testing here
    import sys
    app = QtWidgets.QApplication(sys.argv)
    #sw = QtSvg.QSvgWidget('LatheMacro.svg')
    sw = MacroTab()
    sw.setGeometry(50, 50, 759, 668)
    sw.show()
    sys.exit(app.exec_())
