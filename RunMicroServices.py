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

import sys, os
import argparse

import yaml

from PitchEstimate import PitchEstimate
from GroundRoll import GroundRoll
from RollEstimate import RollEstimate
from RollRateEstimate import RollRateEstimate
from TurnRateComputed import TurnRateComputed
from HeadingComputed import HeadingComputed
from Pitch import Pitch
from Roll import Roll
from Yaw import Yaw
from Heading import Heading
from RollRate import RollRate
from HeadingTasEstimate import HeadingTasEstimate
from WindEstimate import WindEstimate
from PressureFactors import PressureFactors
from AirspeedComputed import AirspeedComputed
from AirspeedEstimate import AirspeedEstimate
from AltitudeComputed import AltitudeComputed
from TrackRate import TrackRate
from TurnRate import TurnRate
from Airspeed import Airspeed
from Altitude import Altitude
from ClimbRate import ClimbRate
from GroundVector import GroundVector
from ClimbRateEstimate import ClimbRateEstimate
from PitchRate import PitchRate
import InternalPublisher
import MicroServerComs
from PubSub import CONFIG_FILE

def run_service(so):
    so.listen()

if __name__ == "__main__":
    opt = argparse.ArgumentParser(description=
            'Run the microservices necessary for a complete, self checking AHRS computation pipeline')
    opt.add_argument('-p', '--pubsub-config', default=CONFIG_FILE,
            help='YAML config file coms configuration')
    opt.add_argument('-a', '--airspeed-config', default='airspeed_curve.yml',
            help='YAML config file pressure differential->airspeed curve')
    opt.add_argument('-m', '--heading-calibration', default='heading_calibration.yml',
            help='YAML config file magnetic heading calibration curve')
    opt.add_argument('-r', '--pressure-calibration', default='pressure_calibration.yml',
            help='YAML config file altitude calibration curve')
    opt.add_argument('-c', '--accelerometer-calibration', default='accelerometer_calibration.yml',
            help='YAML config file accelerometer calibration curve')
    args = opt.parse_args()

    with open (args.pubsub_config, 'r') as yml:
        MicroServerComs._pubsub_config = yaml.load(yml)
        yml.close()
    InternalPublisher.TheInternalPublisher = InternalPublisher.InternalPublisher(
            MicroServerComs._pubsub_config)
    airspeed_config = None
    if os.path.exists(args.airspeed_config):
        with open (args.airspeed_config, 'r') as yml:
            airspeed_config = yaml.load(yml)
            yml.close()
    heading_calibration = None
    if os.path.exists(args.heading_calibration):
        with open (args.heading_calibration, 'r') as yml:
            heading_calibration = yaml.load(yml)
            yml.close()
    pressure_calibration = None
    if os.path.exists(args.pressure_calibration):
        with open (args.pressure_calibration, 'r') as yml:
            pressure_calibration = yaml.load(yml)
            yml.close()
    accelerometer_calibration = None
    if os.path.exists(args.accelerometer_calibration):
        with open (args.accelerometer_calibration, 'r') as yml:
            accelerometer_calibration = yaml.load(yml)
            yml.close()

    service_objects = [
                 PitchEstimate(accelerometer_calibration)
                ,GroundRoll(accelerometer_calibration)
                ,RollEstimate()
                ,RollRateEstimate()
                ,TurnRateComputed()
                ,HeadingComputed(heading_calibration)
                ,Pitch()
                ,Roll()
                ,Yaw()
                ,Heading()
                ,RollRate()
                ,HeadingTasEstimate()
                ,WindEstimate()
                ,PressureFactors(pressure_calibration)
                ,AirspeedComputed(airspeed_config)
                ,AirspeedEstimate()
                ,AltitudeComputed(pressure_calibration)
                ,TrackRate()
                ,TurnRate()
                ,Airspeed()
                ,Altitude()
                ,ClimbRate()
                ,GroundVector()
                ,ClimbRateEstimate()
                ,PitchRate()
                ]
    InternalPublisher.TheInternalPublisher.listen()
