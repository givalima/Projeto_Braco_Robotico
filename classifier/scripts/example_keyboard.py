from RobotArm import *
from msvcrt import getch

"""
Example of how to control robot using keyboard with getch (only works in Windows).
Receive as parameter the step of movement in millimeters.
"""
def keyboard_com(robot_obj, step):
    print('[COMMAND] Keyboard controller. Use Arrows to move in plane, Q and A to move in depth.')
    while True:
        key = (ord(getch()))
        move_direction = [0,0,0]
        if key == 75:
            print('[KEY] Arrow Left (Y-)')
            move_direction = [0,-step,0]
        elif key == 77:
            print('[KEY] Arrow Right (Y+)')
            move_direction = [0,step,0]
        elif key == 72:
            print('[KEY] Arrow Up (X-)')
            move_direction = [-step,0,0]
        elif key == 80:
            print('[KEY] Arrow Down (X+)')
            move_direction = [step,0,0]
        elif key == 113:
            print('[KEY] Q (Z+)')
            move_direction = [0,0,step]
        elif key == 97:
            print('[KEY] A (Z-)')
            move_direction = [0,0,-step]
        elif key == 32:
            print('[KEY] SpaceBar Toggle Gripper')
            robot_obj.set_gripper_state( int(not robot_obj.gripper_state) )
        elif key == 120:    # x to quit
            print('[KEY] X Quitting')
            break
 
        # Check if a movement key was pressed
        if norm(move_direction) <= 0:
            continue
 
        # Detect current position
        robot_joints = robot_obj.robot.Joints()
        robot_pose = robot_obj.robot.SolveFK(robot_joints)
 
        # Move robot
        new_robot_pose = transl( move_direction )*robot_pose
        robot_obj.set_pose_rel_base( new_robot_pose )
 
        # Print absolute gripper position
        gripper_position = robot_obj.gripper_models[0].PoseAbs().Pos()
        print('[POS] Gripper absolute position:', gripper_position)

if __name__ == '__main__':
    print('Hello World!')
    robot_obj = RobotArm()
    keyboard_com(robot_obj, 5)