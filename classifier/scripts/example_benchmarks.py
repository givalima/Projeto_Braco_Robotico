from RobotArm import *

"""
Simulates XYZ data received from Kinect that varies between -max_val and max_val.
set_p_rel_kinect() function scales and translates received point by s and p0 respectively, converting to millimeters. 
Transformation parameters s and p0 are set via set_map_transform() function.
Final destination is calculated relative to start_frame.
"""
def benchmark_kinect(robot_obj, iterations):
    accum_time = 0
    max_val = 0.5
    # Open gripper
    robot_obj.set_gripper_state( 0 )
    print('[BENCHMARK_KINECT] Starting kinect points benchmark with {} iterations.'.format(iterations))
    init_time = time()
    for i in range(0, iterations):
        start_time = time()
        # Calculate new target position
        p = [uniform(-max_val, max_val), uniform(-max_val, max_val), uniform(-max_val, max_val)]
        # Set position
        robot_obj.set_p_rel_kinect(p)
        # Check time
        diff_time = time() - start_time
        accum_time += diff_time
    print('[BENCHMARK_KINECT] Average time for {} iterations was of {:.4f} seconds.'.format(iterations, accum_time/iterations))
    finish_time = time()
    print('[BENCHMARK_KINECT] Completion took {:.4f} seconds.'.format(finish_time - init_time))

"""
Simulates XYZ data that varies between -max_val and max_val in millimeters.
set_p_rel_start() function receives points in millimeters.
Final destination is calculated relative to start_frame.
"""
def benchmark_pose(robot_obj, iterations):
    accum_time = 0
    max_val = 80
    # Open gripper
    robot_obj.set_gripper_state( 0 )
    print('[BENCHMARK_POSE] Starting pose benchmark with {} iterations.'.format(iterations))
    init_time = time()
    for i in range(0, iterations):
        start_time = time()
        # Calculate new target position
        p = [randint(-max_val, max_val), randint(-max_val, max_val), randint(-max_val, max_val)]
        # Set position
        robot_obj.set_p_rel_start(p)
        # Check time
        diff_time = time() - start_time
        accum_time += diff_time
    print('[BENCHMARK_POSE] Average time for {} iterations was of {:.4f} seconds.'.format(iterations, accum_time/iterations))
    finish_time = time()
    print('[BENCHMARK_POSE] Completion took {:.4f} seconds.'.format(finish_time - init_time))

"""
Simulates transition between gripper states. 
"""
def benchmark_gripper_state(robot_obj, iterations):
    # Open gripper
    robot_obj.set_gripper_state( 0 )
    robot_obj.robot.MoveJ( robot_obj.rest_frame )
    robot_obj.robot.MoveJ( robot_obj.start_frame )
    print('[BENCHMARK_GRIPPER] Starting gripper benchmark with {} iterations.'.format(iterations))
    accum_time = 0
    init_time = time()
    # Check num of possible gripper states
    n_states = len(robot_obj.gripper_models)
    for i in range(0, iterations):
        start_time = time()
        # Generate a new state within gripper states
        state = randint(0, n_states-1)
        robot_obj.set_gripper_state( state )
        diff_time = time() - start_time
        #print('[BENCHMARK_GRIPPER] Gripper update to state {} took {:.4f} seconds'.format(state, diff_time))
        accum_time += diff_time
    print('[BENCHMARK_GRIPPER] Average time for {} iterations was of {:.4f} seconds.'.format(iterations, accum_time/iterations))
    finish_time = time()
    print('[BENCHMARK_GRIPPER] Completion took {:.4f} seconds.'.format(finish_time - init_time))

"""
Simulates going to a cube position assuming that robot pose is unknown.
Include calculations to find current robot pose. Chooses a cube randomically to grab.
Make transformation from cube coordinates from origin to robot base frame.
Then, calculates inverse kinematics to find new joint states.
"""
def benchmark_gripper_grab_n_release(robot_obj, iterations):
    print('[BENCHMARK_GRAB] Starting gripper benchmark with {} iterations.'.format(iterations))
    accum_time = 0
    init_time = time()
    for iteration in range(0, iterations):
        start_time = time()
        # Calculate cube position relative to tool
        robot_joints =  robot_obj.robot.Joints()
        # Find current gripper position relative to base
        pose_robot_rel_base = robot_obj.robot.SolveFK(robot_joints)
        # Find cube position relative to base
        pose_cube_rel_org = robot_obj.handy_parts[ randint(0,2) ].PoseAbs()
        pose_base_rel_org = robot_obj.base_frame.PoseAbs()
        pose_cube_rel_base = invH( pose_base_rel_org ) * pose_cube_rel_org
        #print('[BENCHMARK_GRAB] Pose robot rel base: \n', pose_robot_rel_base)
        #print('[BENCHMARK_GRAB] Pose cube rel base: \n', pose_cube_rel_base)

        # Move
        robot_obj.set_pose_rel_base( RelTool(pose_cube_rel_base, 0, 0, -130) )
        # Close gripper
        robot_obj.set_gripper_state( 1 )
        # Open gripper
        robot_obj.set_gripper_state( 0 )

        diff_time = time() - start_time
        accum_time += diff_time
    print('[BENCHMARK_GRAB] Average time for {} iterations was of {:.4f} seconds.'.format(iterations, accum_time/iterations))
    finish_time = time()
    print('[BENCHMARK_GRAB] Completion took {:.4f} seconds.'.format(finish_time - init_time))
     
if __name__ == '__main__':
    robot_obj = RobotArm()
    robot_obj.set_map_transform([210, 210, 210], [0, 0, 0] )

    benchmark_kinect(robot_obj, 500)
    benchmark_pose(robot_obj, 500)
    benchmark_gripper_state(robot_obj, 500)
    benchmark_gripper_grab_n_release(robot_obj, 500)
