# Copyright (C) 2018  Garrett Herschleb
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>


import Globals
from MicroServerComs import MicroServerComs

class Pitch(MicroServerComs):
    def __init__(self, cor_rate=1.0, conf_mult=1.0):
        MicroServerComs.__init__(self, "Pitch")
        self.last_time = None
        self.pitch = 0.0
        self.pitch_estimate = None
        self.vertical = None
        self.flight_mode = Globals.FLIGHT_MODE_GROUND
        self.pitch_confidence = 0.0
        # Default 1 degree per minute
        self.correction_rate = cor_rate / 60.0
        self.confidence_multiplier = conf_mult

    def updated(self, channel):
        if channel == 'rotationsensors':
            if self.last_time and self.flight_mode != Globals.FLIGHT_MODE_GROUND:
                timediff = self.rotationsensors_updated - self.last_time
                correction_factor = self.correction_rate * timediff
                if self.pitch_estimate is not None or \
                        (self.vertical is None or (not self.vertical)):
                    self.pitch += (self.r_x * timediff +
                                (self.pitch_estimate - self.pitch) * correction_factor)
                else:
                    self.pitch += (self.r_x * timediff)
                    self.pitch_confidence = 5.0
                if self.pitch_estimate is not None or \
                        (self.vertical is None or (not self.vertical)):
                    variance = abs(self.pitch - self.pitch_estimate)
                    self.pitch_confidence = 10.0 - variance * self.confidence_multiplier
                self.timestamp = self.rotationsensors_updated
                self.publish ()
                print ("Pitch: %g => %g(%g)"%(self.r_x, self.pitch, self.pitch_confidence))
            self.last_time = self.rotationsensors_updated
        elif channel == 'PitchEstimate':
            if self.flight_mode == Globals.FLIGHT_MODE_GROUND:
                self.pitch = self.pitch_estimate
                self.pitch_confidence = 10.0 - self.pitch * 0.5
                self.timestamp = self.PitchEstimate_updated
                self.publish ()
                print ("Pitch: (from estimate) %g => %g(%g)"%(self.pitch_estimate, self.pitch, self.pitch_confidence))


if __name__ == "__main__":
    pitch = Pitch()
    pitch.listen()
