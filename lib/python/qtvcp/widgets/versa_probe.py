#!/usr/bin/env python
# Qtvcp versa probe
#
# Copyright (c) 2018  Chris Morley <chrisinnanaimo@hotmail.com>
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
# a probe screen based on Versa probe screen

import sys
import os
import hal

from PyQt5 import QtGui, QtCore, QtWidgets, uic
from PyQt5.QtCore import QProcess, QByteArray

from qtvcp.widgets.widget_baseclass import _HalWidgetBase
from qtvcp.core import Status, Action, Info
from qtvcp import logger
# Instantiate the libraries with global reference
# STATUS gives us status messages from linuxcnc
# LOG is for running code logging
STATUS = Status()
ACTION = Action()
INFO = Info()
LOG = logger.getLogger(__name__)

current_dir =  os.path.dirname(__file__)
SUBPROGRAM = os.path.abspath(os.path.join(current_dir, 'versa_probe_subprog.py'))
HELP = os.path.join(INFO.LIB_PATH,'widgets_ui', 'versa_usage.html')

class VersaProbe(QtWidgets.QWidget, _HalWidgetBase):
    def __init__(self, parent=None):
        super(VersaProbe, self).__init__(parent)
        if INFO.MACHINE_IS_METRIC:
            self.valid = QtGui.QDoubleValidator(0.0, 999.999, 3)
        else:
            self.valid = QtGui.QDoubleValidator(0.0, 99.9999, 4)
        self.setMinimumSize(600, 420)
        # Load the widgets UI file:
        self.filename = os.path.join(INFO.LIB_PATH,'widgets_ui', 'versa_probe.ui')
        try:
            self.instance = uic.loadUi(self.filename, self)
        except AttributeError as e:
            LOG.critical(e)
        self.process_busy = False
        self.input_data = ['adj_x','adj_y','adj_z','adj_angle',
                           'probe_diam','max_travel','latch_return_dist',
                           'search_vel','probe_vel','rapid_vel',
                           'side_edge_length', 'tool_probe_height','tool_block_height',
                           'xy_clearances','z_clearance']

        for i in self.input_data:
            self['input_' + i].setValidator(self.valid)
        # button connections
        self.inside_buttonGroup.buttonClicked.connect(self.btn_inside_clicked)
        self.outside_buttonGroup.buttonClicked.connect(self.btn_outside_clicked)
        self.skew_buttonGroup.buttonClicked.connect(self.btn_skew_clicked)
        self.length_buttonGroup.buttonClicked.connect(self.btn_length_clicked)

    def _hal_init(self):
        def homed_on_test():
            return (STATUS.machine_is_on() and (STATUS.is_all_homed() or INFO.NO_HOME_REQUIRED))

        STATUS.connect('state-off', lambda w: self.setEnabled(False))
        STATUS.connect('state-estop', lambda w: self.setEnabled(False))
        STATUS.connect('interp-idle', lambda w: self.setEnabled(homed_on_test()))
        STATUS.connect('all-homed', lambda w: self.setEnabled(True))
        STATUS.connect('error', self.send_error)
        STATUS.connect('periodic', lambda w: self.check_probe())

        if self.PREFS_:
            self.input_search_vel.setText(str(self.PREFS_.getpref( "ps_searchvel", 300.0, float, 'VERSA_PROBE_OPTIONS')) )
            self.input_probe_vel.setText(str(self.PREFS_.getpref( "ps_probevel", 10.0, float, 'VERSA_PROBE_OPTIONS')) )
            self.input_z_clearance.setText(str(self.PREFS_.getpref( "ps_z_clearance", 3.0, float, 'VERSA_PROBE_OPTIONS')) )
            self.input_max_travel.setText(str(self.PREFS_.getpref( "ps_probe_max", 1.0, float, 'VERSA_PROBE_OPTIONS')) )
            self.input_latch_return_dist.setText(str(self.PREFS_.getpref( "ps_probe_latch", 0.5, float, 'VERSA_PROBE_OPTIONS')) )
            self.input_probe_diam.setText(str(self.PREFS_.getpref( "ps_probe_diam", 2.0, float, 'VERSA_PROBE_OPTIONS')) )
            self.input_xy_clearances.setText(str(self.PREFS_.getpref( "ps_xy_clearance", 5.0, float, 'VERSA_PROBE_OPTIONS')) )
            self.input_side_edge_length.setText(str(self.PREFS_.getpref( "ps_side_edge_length", 5.0, float, 'VERSA_PROBE_OPTIONS')) )
            self.input_tool_probe_height.setText(str(self.PREFS_.getpref( "ps_probe_height", 20.0, float, 'VERSA_PROBE_OPTIONS')) )
            self.input_tool_block_height.setText(str(self.PREFS_.getpref( "ps_block_height", 20.0, float, 'VERSA_PROBE_OPTIONS')) )
            self.input_adj_x.setText(str(self.PREFS_.getpref( "ps_offs_x", 0.0, float, 'VERSA_PROBE_OPTIONS')) )
            self.input_adj_y.setText(str(self.PREFS_.getpref( "ps_offs_y", 0.0, float, 'VERSA_PROBE_OPTIONS')) )
            self.input_adj_z.setText(str(self.PREFS_.getpref( "ps_offs_z", 0.0, float, 'VERSA_PROBE_OPTIONS')) )
            self.input_adj_angle.setText(str(self.PREFS_.getpref( "ps_offs_angle", 0.0, float, 'VERSA_PROBE_OPTIONS')) )
            self.input_rapid_vel.setText(str(self.PREFS_.getpref( "ps_probe_rapid_vel", 60.0, float, 'VERSA_PROBE_OPTIONS')) )

        self.read_page_data()
        self.start_process()

    # when qtvcp closes this gets called
    def closing_cleanup__(self):
        if self.PREFS_:
            LOG.debug('Saving Versa probe data to preference file.')
            self.PREFS_.putpref( "ps_searchvel", float(self.input_search_vel.text()), float, 'VERSA_PROBE_OPTIONS')
            self.PREFS_.putpref( "ps_probevel", float(self.input_probe_vel.text()), float, 'VERSA_PROBE_OPTIONS')
            self.PREFS_.putpref( "ps_z_clearance", float(self.input_z_clearance.text()), float, 'VERSA_PROBE_OPTIONS')
            self.PREFS_.putpref( "ps_probe_max", float(self.input_max_travel.text()), float, 'VERSA_PROBE_OPTIONS')
            self.PREFS_.putpref( "ps_probe_latch", float(self.input_latch_return_dist.text()), float, 'VERSA_PROBE_OPTIONS')
            self.PREFS_.putpref( "ps_probe_diam", float(self.input_probe_diam.text()), float, 'VERSA_PROBE_OPTIONS')
            self.PREFS_.putpref( "ps_xy_clearance", float(self.input_xy_clearances.text()), float, 'VERSA_PROBE_OPTIONS')
            self.PREFS_.putpref( "ps_side_edge_length", float(self.input_side_edge_length.text()), float, 'VERSA_PROBE_OPTIONS')
            self.PREFS_.putpref( "ps_probe_height", float(self.input_tool_probe_height.text()), float, 'VERSA_PROBE_OPTIONS')
            self.PREFS_.putpref( "ps_block_height", float(self.input_tool_block_height.text()), float, 'VERSA_PROBE_OPTIONS')
            self.PREFS_.putpref( "ps_offs_x", float(self.input_adj_x.text()), float, 'VERSA_PROBE_OPTIONS')
            self.PREFS_.putpref( "ps_offs_y", float(self.input_adj_y.text()), float, 'VERSA_PROBE_OPTIONS')
            self.PREFS_.putpref( "ps_offs_z", float(self.input_adj_z.text()), float, 'VERSA_PROBE_OPTIONS')
            self.PREFS_.putpref( "ps_offs_angle", float(self.input_adj_angle.text()), float, 'VERSA_PROBE_OPTIONS')
            self.PREFS_.putpref( "ps_probe_rapid_vel", float(self.input_rapid_vel.text()), float, 'VERSA_PROBE_OPTIONS')
        self.proc.terminate()

#############################################
# process control
#############################################
    def start_process(self):
        self.proc = QProcess()
        self.proc.setReadChannel(QProcess.StandardOutput)
        self.proc.started.connect(self.process_started)
        self.proc.readyReadStandardOutput.connect(self.read_stdout)
        self.proc.readyReadStandardError.connect(self.read_stderror)
        self.proc.finished.connect(self.process_finished)
        self.proc.start('python {}'.format(SUBPROGRAM))
        return

    def start_probe(self, cmd):
        if self.process_busy is True:
            LOG.error("Probing processor is busy")
            return
        result = self.read_page_data()
        if not result:
            LOG.error("Error reading page data")
            return
        # clear all previous offsets
        ACTION.CALL_MDI("G10 L2 P0 X0 Y0 Z0")
        string_to_send = cmd.encode('utf-8') + ' ' + result + '\n'
#        print("String to send ", string_to_send)
        self.proc.writeData(string_to_send)
        self.process_busy = True

    def process_started(self):
        LOG.info("Versa_Probe started with PID {}\n".format(self.proc.processId()))

    def read_stdout(self):
        qba = self.proc.readAllStandardOutput()
        line = qba.data().encode('utf-8')
        self.parse_input(line)
        self.process_busy = False

    def read_stderror(self):
        qba = self.proc.readAllStandardError()
        line = qba.data().encode('utf-8')
        self.parse_input(line)

    def process_finished(self):
        print("Versa_Probe Process signals finished")

    def parse_input(self, line):
        self.process_busy = False
        if "ERROR" in line:
            print(line)
        elif "DEBUG" in line:
            print(line)
        elif "INFO" in line:
            print(line)
        elif "STATUS" in line:
            line = line.rstrip('\n')
            temp = line.strip('[STATUS] ').split(',')
            for num, i in enumerate(['xm', 'xc', 'xp', 'ym', 'yc', 'yp', 'lx', 'ly', 'z', 'd', 'a']):
                self['status_' + i].setText(temp[num])
            LOG.info("Probing routine completed without errors")
        elif "HISTORY" in line:
            temp = line.strip('[HISTORY]')
            STATUS.emit('update-machine-log', temp, 'TIME')
            LOG.info("Probe history updated to machine log")
        else:
            LOG.error("Error parsing return data from sub_processor. Line={}".format(line))

    def send_error(self, w, kind, text):
        message ='_ErroR_ {},{} \n'.format(kind,text)
        self.proc.writeData(message)

#####################################################
# button callbacks
#####################################################

    def help_clicked(self):
        self.pop_help()

    def btn_outside_clicked(self, button):
        cmd = button.property('probe')
#        print("Button clicked ", cmd)
        self.start_probe(cmd)

    def btn_inside_clicked(self, button):
        cmd = button.property('probe')
#        print("Button clicked ", cmd)
        self.start_probe(cmd)

    def btn_skew_clicked(self, button):
        cmd = button.property('probe')
#        print("Button clicked ", cmd)
        self.start_probe(cmd)

    def btn_length_clicked(self, button):
        cmd = button.property('probe')
#        print("Button clicked ", cmd)
        self.start_probe(cmd)

    ###### set origin offset ######################
    def pbtn_set_x_released(self):
        ACTION.SET_AXIS_ORIGIN('X', float(self.input_adj_x.text()))

    def pbtn_set_y_released(self):
        ACTION.SET_AXIS_ORIGIN('Y', float(self.input_adj_y.text()))

    def pbtn_set_z_released(self):
        ACTION.SET_AXIS_ORIGIN('Z', float(self.input_adj_z.text()))

    def pbtn_set_angle_released(self):
        self.status_a = "%.3f" % float(self.input_adj_angle.text())
        s="G10 L2 P0"
        if self.pbtn_allow_auto_zero.isChecked():
            s +=  " X%.4f"% float(self.input_adj_x.text())
            s +=  " Y%.4f"% float(self.input_adj_y.text())
        else :
            a = STATUS.get_probed_position_with_offsets()
            s +=  " X%.4f"%a[0]
            s +=  " Y%.4f"%a[1]
        s +=  " R%.4f"% float(self.input_adj_angle.text())
        ACTION.CALL_MDI_WAIT(s, 30)

#####################################################
# Helper functions
#####################################################
    def read_page_data(self):
        arg = ''
        for i in ['adj_x', 'adj_y', 'adj_z', 'adj_angle',
                   'probe_diam', 'max_travel', 'latch_return_dist',
                   'search_vel', 'probe_vel', 'rapid_vel',
                   'side_edge_length', 'tool_probe_height', 'tool_block_height',
                   'xy_clearances', 'z_clearance']:
            arg += '{},'.format(float(self['input_' + i].text()))
        arg += str(self.pbtn_allow_auto_zero.isChecked())
        arg += ','
        arg += str(self.pbtn_allow_auto_skew.isChecked())
        return arg
        
    def check_probe(self):
        self.led_probe_function_chk.setState(hal.get_value('motion.probe-input'))

    def pop_help(self):
        d = QtWidgets.QDialog()
        d.setMinimumWidth(600)
        l = QtWidgets.QVBoxLayout()
        t = QtWidgets.QTextEdit()
        t.setReadOnly(False)
        l.addWidget(t)

        bBox = QtWidgets.QDialogButtonBox()
        bBox.addButton('Ok', QtWidgets.QDialogButtonBox.AcceptRole)
        bBox.accepted.connect(d.accept)
        l.addWidget(bBox)
        d.setLayout(l)

        try:
            file = QtCore.QFile(HELP)
            file.open(QtCore.QFile.ReadOnly)
            html = file.readAll()
            html = unicode(html, encoding='utf8')
            html = html.replace("../images/probe_icons/","{}/probe_icons/".format(INFO.IMAGE_PATH))
            t.setHtml(html)
        except Exception as e:
            t.setText('Versa Probe Help file Unavailable:\n\n{}'.format(e))

        d.show()
        d.exec_()

########################################
# required boiler code
########################################
    def __getitem__(self, item):
        return getattr(self, item)
    def __setitem__(self, item, value):
        return setattr(self, item, value)

####################################
# Testing
####################################
if __name__ == "__main__":
    from PyQt5.QtWidgets import *
    from PyQt5.QtCore import *
    from PyQt5.QtGui import *

    app = QtWidgets.QApplication(sys.argv)
    w = VersaProbe()
    w.show()
    sys.exit( app.exec_() )

