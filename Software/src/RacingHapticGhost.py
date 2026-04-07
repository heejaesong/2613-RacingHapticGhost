import ac
import acsys
import os

# Global label handles
status_label = None
brake_label = None
throttle_label = None
steering_label = None

# File where AC app will publish its current mode/data
data_file_path = None

def acMain(ac_version):
    global status_label, brake_label, throttle_label, steering_label, data_file_path

    appWindow = ac.newApp("RacingHapticGhost")
    ac.setSize(appWindow, 260, 140)

    ac.setTitle(appWindow, "RacingHapticGhost")

    status_label = ac.addLabel(appWindow, "Live: ")
    ac.setPosition(status_label, 10, 30)

    brake_label = ac.addLabel(appWindow, "Brake: ")
    ac.setPosition(brake_label, 10, 55)

    throttle_label = ac.addLabel(appWindow, "Throttle: ")
    ac.setPosition(throttle_label, 10, 80)

    steering_label = ac.addLabel(appWindow, "Steering: ")
    ac.setPosition(steering_label, 10, 105)

    # Save file beside this script
    data_file_path = os.path.join(os.path.dirname(__file__), "ac_state.txt")

    return "RacingHapticGhost"


def acUpdate(deltaT):
    global data_file_path

    car_id = 0

    is_live = ac.isAcLive()
    brake = ac.getCarState(car_id, acsys.CS.Brake)
    throttle = ac.getCarState(car_id, acsys.CS.Gas)
    steering = ac.getCarState(car_id, acsys.CS.Steer) / 507

    ac.setText(status_label, "Live: {}".format(is_live))
    ac.setText(brake_label, "Brake: {:.3f}".format(brake))
    ac.setText(throttle_label, "Throttle: {:.3f}".format(throttle))
    ac.setText(steering_label, "Steering: {:.3f}".format(steering))

    f = open(data_file_path, "w")
    f.write("{},{:.3f},{:.3f},{:.3f}\n".format(int(is_live), steering, throttle, brake))
    f.close()