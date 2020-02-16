#!/usr/bin/env python

'''
w_bolt_circle.py

Copyright (C) 2019, 2020  Phillip A Carter

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
'''

import os
import gtk
import time
import math
import linuxcnc
import shutil
import hal
from subprocess import Popen,PIPE

class bolt_circle:

    def __init__(self):
        self.i = linuxcnc.ini(os.environ['INI_FILE_NAME'])
        self.c = linuxcnc.command()
        self.s = linuxcnc.stat()
        self.gui = self.i.find('DISPLAY', 'DISPLAY').lower()
        self.configFile = '{}_wizards.cfg'.format(self.i.find('EMC', 'MACHINE').lower())

    def dialog_error(self, error):
        md = gtk.MessageDialog(self.W, 
            gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_ERROR, 
            gtk.BUTTONS_CLOSE, error)
        md.run()
        md.destroy()

    def load_file(self, fName):
        if self.gui == 'axis':
            Popen('axis-remote {}'.format(fName), stdout = PIPE, shell = True)
        elif self.gui == 'gmoccapy':
            self.c = linuxcnc.command()
            self.c.program_open('blank.ngc')
            self.c.program_open(fName)
        else:
            print('Unknown GUI in .ini file')

    def end_this_shape(self, event):
        if os.path.exists(self.fWizard):
            outWiz = open(self.fWizard, 'a+')
            post = False
            for line in outWiz:
                if '(postamble)' in line:
                    post = True
            if not post:
                outWiz.write('\n(postamble)\n')
                outWiz.write('{}\n'.format(self.postamble))
                outWiz.write('m30\n')
            outWiz.close()
            self.load_file(self.fWizard)
        self.W.destroy()
        return None

    def add_shape_to_file(self, event):
        if os.path.exists(self.fWizard):
            path = os.path.dirname(os.path.abspath(self.fWizard))
            tmp = ('{}/tmp'.format(path))
            shutil.copyfile(self.fWizard, tmp)
            inWiz = open(tmp, 'r')
            outWiz = open(self.fWizard, 'w')
            for line in inWiz:
                if '(postamble)' in line:
                    break
                outWiz.write(line)
            inWiz.close()
            outWiz.close()
            os.remove(tmp)
            inTmp = open(self.fTmp, 'r')
            outWiz = open(self.fWizard, 'a')
            for line in inTmp:
                outWiz.write(line)
        else:
            inTmp = open(self.fTmp, 'r')
            outWiz = open(self.fWizard, 'w')
            outWiz.write('{}\n'.format(self.preamble))
            outWiz.write('f#<_hal[plasmac.cut-feed-rate]>\n')
            for line in inTmp:
                outWiz.write(line)
        inTmp.close()
        outWiz.close()
        self.add.set_sensitive(False)

    def send_preview(self, event):
        self.s.poll()
        xPos = self.s.actual_position[0] - self.s.g5x_offset[0] - self.s.g92_offset[0]
        yPos = self.s.actual_position[1] - self.s.g5x_offset[1] - self.s.g92_offset[1]
        if self.dEntry.get_text():
            cRadius = float(self.dEntry.get_text()) / 2
        else:
            cRadius = 0
        if self.hdEntry.get_text():
            hRadius = float(self.hdEntry.get_text()) / 2
        else:
            hRadius = 0
        if self.hEntry.get_text():
            holes = int(self.hEntry.get_text())
        else:
            holes = 0
        if cRadius > 0 and hRadius > 0 and holes > 0:
            right = math.radians(0)
            up = math.radians(90)
            left = math.radians(180)
            down = math.radians(270)
            if hRadius < self.scRadius:
                sHole = True
            else:
                sHole = False
            if self.aEntry.get_text():
                angle = math.radians(float(self.aEntry.get_text()))
            else:
                angle = 0
            if self.liEntry.get_text():
                leadIn = float(self.liEntry.get_text())
                leadInOffset = leadIn * math.sin(math.radians(45))
            else:
                leadIn = 0
                leadInOffset = 0
            if leadIn > hRadius:
                leadIn = hRadius
            if leadInOffset > hRadius:
                leadInOffset = hRadius
            hAngle = math.radians(360 / float(holes))
            if self.xSEntry.get_text():
                if self.centre.get_active():
                    xC = float(self.xSEntry.get_text())
                else:
                    xC = float(self.xSEntry.get_text()) + cRadius
            else:
                if self.centre.get_active():
                    xC = xPos
                else:
                    xC = xPos + cRadius
            if self.ySEntry.get_text():
                if self.centre.get_active():
                    yC = float(self.ySEntry.get_text())
                else:
                    yC = float(self.ySEntry.get_text()) + cRadius
            else:
                if self.centre.get_active():
                    yC = yPos
                else:
                    yC = yPos + cRadius
            self.fTmp = '{}/shape.tmp'.format(self.tmpDir)
            self.fNgc = '{}/shape.ngc'.format(self.tmpDir)
            outTmp = open(self.fTmp, 'w')
            outNgc = open(self.fNgc, 'w')
            if os.path.exists(self.fWizard):
                inWiz = open(self.fWizard, 'r')
                for line in inWiz:
                    if '(postamble)' in line:
                        break
                    outNgc.write(line)
            else:
                outNgc.write('{}\n'.format(self.preamble))
                outNgc.write('f#<_hal[plasmac.cut-feed-rate]>\n')
            for hole in range(holes):
                outTmp.write('\n(wizard bolt circle, hole #{})\n'.format(hole + 1))
                xhC = xC + cRadius * math.cos(hAngle * hole + angle)
                yhC = yC + cRadius * math.sin(hAngle * hole + angle)
                xS = xhC + hRadius * math.cos(left)
                yS = yhC + hRadius * math.sin(left)
                if sHole:
                    outTmp.write('m67 E3 Q{}\n'.format(self.hSpeed))
                    xlStart = xS + leadIn * math.cos(right)
                    ylStart = yS + leadIn * math.sin(right)
                    outTmp.write('g0 x{:.6f} y{:.6f}\n'.format(xlStart, ylStart))
                    if self.offset.get_active():
                        outTmp.write('g41.1 d#<_hal[plasmac_run.kerf-width-f]>\n')
                    outTmp.write('m3 $0 s1\n')
                    outTmp.write('g1 x{:.6f} y{:.6f}\n'.format(xS, yS))
                else:
                    xlCentre = xS + (leadInOffset * math.cos(angle + right))
                    ylCentre = yS + (leadInOffset * math.sin(angle + right))
                    xlStart = xlCentre + (leadInOffset * math.cos(angle + up))
                    ylStart = ylCentre + (leadInOffset * math.sin(angle + up))
                    outTmp.write('g0 x{:.6f} y{:.6f}\n'.format(xlStart, ylStart))
                    if self.offset.get_active():
                        outTmp.write('g41.1 d#<_hal[plasmac_run.kerf-width-f]>\n')
                    outTmp.write('m3 $0 s1\n')
                    outTmp.write('g3 x{:.6f} y{:.6f} i{:.6f} j{:.6f}\n'.format(xS, yS, xlCentre - xlStart, ylCentre - ylStart))
                outTmp.write('g3 x{:.6f} y{:.6f} i{:.6f}\n'.format(xS, yS, hRadius * math.cos(right)))
                if not sHole:
                    xlEnd = xlCentre + (leadInOffset * math.cos(angle + down))
                    ylEnd = ylCentre + (leadInOffset * math.sin(angle + down))
                    outTmp.write('g3 x{:.6f} y{:.6f} i{:.6f} j{:.6f}\n'.format(xlEnd, ylEnd, xlCentre - xS, ylCentre - yS))
                if self.offset.get_active():
                    outTmp.write('g40\n')
                if sHole:
                    outTmp.write('m67 E3 Q0\n')
                outTmp.write('m5\n')
            outTmp.close()
            outTmp = open(self.fTmp, 'r')
            for line in outTmp:
                outNgc.write(line)
            outTmp.close()
            outNgc.write('\n(postamble)\n')
            outNgc.write('{}\n'.format(self.postamble))
            outNgc.write('m30\n')
            outNgc.close()
            self.load_file(self.fNgc)
            self.add.set_sensitive(True)
            hal.set_p('plasmac_run.preview-tab', '1')
        else:
            msg = ''
            if cRadius == 0:
                msg += 'Diameter is required\n\n'
            if hRadius == 0:
                msg += 'Hole Diameter is required\n\n'
            if holes == 0:
                msg += '# of Holes are required'
            self.dialog_error(msg)

    def do_bolt_circle(self, fWizard, tmpDir):
        self.tmpDir = tmpDir
        self.fWizard = fWizard
        self.scRadius = 0.0
        self.hSpeed = 100
        self.W = gtk.Dialog('Bolt Circle',
                       None,
                       gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                       buttons = None)
        self.W.set_keep_above(True)
        self.W.set_position(gtk.WIN_POS_CENTER_ALWAYS)
        self.W.set_default_size(250, 200)
        t = gtk.Table(1, 1, True)
        t.set_row_spacings(6)
        self.W.vbox.add(t)
        offsetLabel = gtk.Label('Offset')
        offsetLabel.set_alignment(0.95, 0.5)
        offsetLabel.set_width_chars(10)
        t.attach(offsetLabel, 0, 1, 0, 1)
        self.offset = gtk.CheckButton('Kerf Width')
        t.attach(self.offset, 1, 2, 0, 1)
        lLabel = gtk.Label('Lead In')
        lLabel.set_alignment(0.95, 0.5)
        lLabel.set_width_chars(10)
        t.attach(lLabel, 0, 1, 1, 2)
        self.liEntry = gtk.Entry()
        self.liEntry.set_width_chars(10)
        t.attach(self.liEntry, 1, 2, 1, 2)
        loLabel = gtk.Label('Lead Out')
        loLabel.set_alignment(0.95, 0.5)
        loLabel.set_width_chars(10)
        t.attach(loLabel, 0, 1, 2, 3)
        self.loEntry = gtk.Entry()
        self.loEntry.set_width_chars(10)
        t.attach(self.loEntry, 1, 2, 2, 3)
        xSLabel = gtk.Label('X start')
        xSLabel.set_alignment(0.95, 0.5)
        xSLabel.set_width_chars(10)
        t.attach(xSLabel, 0, 1, 3, 4)
        self.xSEntry = gtk.Entry()
        self.xSEntry.set_width_chars(10)
        t.attach(self.xSEntry, 1, 2, 3, 4)
        ySLabel = gtk.Label('Y start')
        ySLabel.set_alignment(0.95, 0.5)
        ySLabel.set_width_chars(10)
        t.attach(ySLabel, 0, 1, 4, 5)
        self.ySEntry = gtk.Entry()
        self.ySEntry.set_width_chars(10)
        t.attach(self.ySEntry, 1, 2, 4, 5)
        self.centre = gtk.RadioButton(None, 'Centre')
        t.attach(self.centre, 1, 2, 5, 6)
        bLeft = gtk.RadioButton(self.centre, 'Bottom Left')
        t.attach(bLeft, 0, 1, 5, 6)
        dLabel = gtk.Label('Diameter')
        dLabel.set_alignment(0.95, 0.5)
        dLabel.set_width_chars(10)
        t.attach(dLabel, 0, 1, 6, 7)
        self.dEntry = gtk.Entry()
        self.dEntry.set_width_chars(10)
        t.attach(self.dEntry, 1, 2, 6, 7)
        hdLabel = gtk.Label('Hole Diameter')
        hdLabel.set_alignment(0.95, 0.5)
        hdLabel.set_width_chars(10)
        t.attach(hdLabel, 0, 1, 7, 8)
        self.hdEntry = gtk.Entry()
        self.hdEntry.set_width_chars(10)
        t.attach(self.hdEntry, 1, 2, 7, 8)
        hLabel = gtk.Label('# of holes')
        hLabel.set_alignment(0.95, 0.5)
        hLabel.set_width_chars(10)
        t.attach(hLabel, 0, 1, 8, 9)
        self.hEntry = gtk.Entry()
        self.hEntry.set_width_chars(10)
        t.attach(self.hEntry, 1, 2, 8, 9)
        aLabel = gtk.Label('Start Angle')
        aLabel.set_alignment(0.95, 0.5)
        aLabel.set_width_chars(10)
        t.attach(aLabel, 0, 1, 9, 10)
        self.aEntry = gtk.Entry()
        self.aEntry.set_width_chars(10)
        self.aEntry.set_text('0')
        t.attach(self.aEntry, 1, 2, 9, 10)
        preview = gtk.Button('Preview')
        preview.connect('pressed', self.send_preview)
        t.attach(preview, 0, 1, 11, 12)
        self.add = gtk.Button('Add')
        self.add.set_sensitive(False)
        self.add.connect('pressed', self.add_shape_to_file)
        t.attach(self.add, 2, 3, 11, 12)
        end = gtk.Button('Return')
        end.connect('pressed', self.end_this_shape)
        t.attach(end, 4, 5, 11, 12)
        pixbuf = gtk.gdk.pixbuf_new_from_file_at_size(
                filename='./wizards/images/bolt-circle.png', 
                width=240, 
                height=240)
        image = gtk.Image()
        image.set_from_pixbuf(pixbuf)
        t.attach(image, 2, 5, 1, 9)
        self.xSEntry.grab_focus()
        self.W.show_all()
        if os.path.exists(self.configFile):
            f_in = open(self.configFile, 'r')
            for line in f_in:
                if line.startswith('preamble'):
                    self.preamble = line.strip().split('=')[1]
                elif line.startswith('postamble'):
                    self.postamble = line.strip().split('=')[1]
                elif line.startswith('lead-in'):
                    self.liEntry.set_text(line.strip().split('=')[1])
                elif line.startswith('lead-out'):
                    self.loEntry.set_text(line.strip().split('=')[1])
                elif line.startswith('hole-diameter'):
                    self.scRadius = float(line.strip().split('=')[1]) / 2
                elif line.startswith('hole-speed'):
                    self.hSpeed = float(line.strip().split('=')[1])
        response = self.W.run()
