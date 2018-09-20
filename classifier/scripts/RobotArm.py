from robolink import *    # RoboDK API
from robodk import *      # Robot toolbox
from time import time
from random import randint, uniform, seed
import csv
import socket, struct

class RobotArm():
    def __init__(self):
        # Start link with RoboDK
        print('[INFO] Initing robot...')
        self.link = Robolink()
        print('[INFO] Robot loaded.')
        # Get the home target and the motion starting target
        self.rest_frame = self.link.Item("Home")
        self.start_frame = self.link.Item("Start")
        # Get objects
        self.robot = self.link.Item('Denso VP-6242')
        self.base_frame = self.link.Item("Denso VP-6242 Base")
        self.conveyor = self.link.Item("Conveyor")
        # Load gripper model references
        self.gripper_models = []
        self.gripper_models.append( self.link.Item('Gripper RobotiQ 85 Opened') )
        self.gripper_models.append( self.link.Item('Gripper RobotiQ 85 Closed') )
        self.gripper_models.append( self.link.Item('Gripper RobotiQ 85 Fully Closed') )
        self.gripper_state = 0
        self.set_gripper_state(0)
        # Load handy parts
        self.handy_parts = []
        self.handy_parts.append( self.link.Item('Cube R') )
        self.handy_parts.append( self.link.Item('Cube G') )
        self.handy_parts.append( self.link.Item('Cube B') )
        # Save first point and scaling parameters
        self.s = None
        self.p0 = None
        # Init camera
        # self.init_camera()
        self.set_gripper_state( 0 )
        self.robot.MoveJ( self.start_frame )
        # Random seed
        seed() 
        # TCP connection variables
        self.tcp_hostname = ''
        self.tcp_port = 0
        self.tcp_socket = None
        self.init_tcp()
    
    def init_camera(self):
        """
        Example of how to load or add cameras to simulation environment.
        Be careful. Cameras make code extremmely slower.
        """
        link = self.link
        link.Cam2D_Close()
        camref = link.Item('Camera Top')
        cam_id = link.Cam2D_Add(camref, 'FOCAL_LENGHT=3 FOV=45 FAR_LENGHT=1500 SIZE=640x480 LIGHT_AMBIENT=white LIGHT_DIFFUSE=black LIGHT_SPECULAR=white')

    def init_tcp(self, tcp_hostname='localhost', tcp_port=17001):
        """
        Initializes variables that describe Denso external TCP server.
        
        @params
            tcp_hostname : string
            tcp_port : int
        """
        self.tcp_hostname = tcp_hostname
        self.tcp_port = tcp_port
        self.tcp_socket = socket.socket()
        try:
            self.tcp_socket.connect((self.tcp_hostname, self.tcp_port))
        except Exception as e: 
            print("[SEND_TCP] Something's wrong with %s:%d. Exception is %s" % (self.tcp_hostname, self.tcp_port, e))
            
    def send_tcp(self, angles):
        """
        Send radian angles to Denso TCP server.
        
        @params
            angles : 1x6 list containing Denso joint angles
        """
        # Convert angles to radians
        rad_angles = []
        for angle in angles.tolist():
            rad_angles.append(angle * pi / 180.0)
        # Map to real arm configuration
        rad_angles[1] = -rad_angles[1]
        rad_angles[2] = -rad_angles[2]
        rad_angles[3] = -rad_angles[3]
        rad_angles[4] = -rad_angles[4]
        
        # Struct data togthere in C like byte array
        data = struct.pack(b'<6d', rad_angles[0], 
                                   rad_angles[1],
                                   rad_angles[2],
                                   rad_angles[3],
                                   rad_angles[4],
                                   rad_angles[5])
        try:
            self.tcp_socket.send(data)
        except Exception as e: 
            print("[SEND_TCP] Something's wrong with %s:%d. Exception is %s" % (self.tcp_hostname, self.tcp_port, e))
        finally:
            print("[SEND_TCP] Sent following data {}.".format(rad_angles))
        
    def valid_pose(self, pose):
        """
        Returns if a given pose is reachable.

        @params
        pose : robodk.Pose
            A 4x4 homogenous matrix relative to robot base frame.

        @returns
        bool
            If given pose is reachable.
        """
        position = pose.Pos()
        # [X, Y, Z]
        # Put robot constraints here
        if position[2] < 100:   # Limit Z in order to grip stay out of table.
            return False
        else:
            return True
 
    def set_map_transform(self, s, p0):
        """
        Defines and stores a transformation operation in a XYZ point. 
        
        This transformation has two purposes: (1) calculates future points with relation to a starting point and
        (2) scales results to a given factor.  Transformation is stored in self.transfomation.

        @params
        s : list
            A 1x3 list containing scales in resulting X, Y and Z dimensions respectively
        p0 : list
            A 1x3 list containing starting point to be considered reference
        """
        # Definition of scale transformation
        scale_transf = eye()            # Create 4x4 identiry matrix
        scale_transf[0, 0] = s[0]       # Set scale factors sx, sy and sz
        scale_transf[1, 1] = s[1]
        scale_transf[2, 2] = s[2]
        # Definition of translation transformation
        translation_transf = eye()       # Create 4x4 identity matrix
        _p0 = mult3(p0, -1)              # Multiply position vector by -1
        translation_transf.setPos( _p0 ) # Set previous p0
        # Result transformation (composition of previous two)
        final_transf = scale_transf * translation_transf # Obtain result transfomation
        # Store into self
        self.transformation = final_transf
        self.p0 = p0
        self.s = s
 
    def transform(self, p):
        """
        Apply self.transformation to p unsing a for loop.
        
        @params
        p : list
            A 1x3 vector containing a XYZ point to be transformed.
        
        @returns
        new_p : list
            1x3 vector containing transformed XYZ
        """
        new_p = []
        for i in range(3):
            new_p.append( (p[i] - self.p0[i])*self.s[i] )
        return new_p
 
    def transform_via_matrix(self, p):
        """
        Apply self.transformation to p unsing matrix multiplication.
        
        @params
        p : list
            A 1x3 vector containing a XYZ point to be transformed.
        
        @returns
        _p : list
            1x3 vector containing transformed XYZ
        """
        # Transform it to a homogeneous 1x4 vector
        _p = p + [1]
        # Performs transformation
        _p = self.transformation * _p
        return _p[:-1]
        
    
    def set_p_rel_start(self, p):
        """
        Tries to set robot flange to position p relative to self.start_frame.
        
        @params
        p : list
            A 1x3 vector containing a XYZ point to be transformed.
        """
        # Calculate new target pose relative to start_frame pose
        new_robot_pose = self.start_frame.Pose()*transl(p)
        # Move it to new pose
        self.set_pose_rel_base( new_robot_pose )
 
    def set_p_rel_base(self, p):
        # Create a pose that will contain point p
        target_pose = eye()
        p[2] += 280
        # Set orientation and position of pose
        target_pose[:-1, :-1] = self.start_frame.Pose()[:-1, :-1]
        target_pose[:-1, -1] = p
        #print('[SET_P_REL_BASE] Trying to set following pose:\n {}\n'.format(target_pose))
        self.set_pose_rel_base(target_pose)
        
        
    def set_p_rel_kinect(self, p):
        """
        Tries to set robot flange to a position mapped_p relative to self.start_frame.
        mapped_p, on the other hand, is calculated transforming p by self.transformation.
        
        @params
        p : list
            A 1x3 vector containing a XYZ point to be transformed.
        """
        #mapped_p = self.transform(p)
        #self.set_p_rel_start(mapped_p[][])
        
        [x, y, z] = self.transform(p)
        mapped_p = [p[0] * 350, p[1] * 350, p[2] * 350]#[ z, x, 200 ]
        self.set_p_rel_base(mapped_p)
 
    def set_pose_rel_base(self, pose):
        """
        Tries to set flange to a pose relative to robot base frame.
        
        First, checks constraints defined in self.valid_pose(). Next step is to check if pose is reacheble by robot.
        Finally, check if pose is not close to a singularity. If so, returns True.
        
        @params
        pose : robodk.Pose
            A 4x4 homogenous matrix relative to robot base frame.

        @returns
        bool
            If given pose was reached.
        """       
        # Check if pose is not constrained
        if not self.valid_pose(pose):
            print('[SET_XYZ_REL_START] Position is LOL{}LOL too far, out of \
                  reach or close to a singularity'.format(pose.Pos()))
            return False
 
        new_robot_joints = self.robot.SolveIK(pose)
        # Check if valid
        if len(new_robot_joints.tolist()) < 6:
            print('[SET_XYZ_REL_START] Position is LOL{}LOL too far, out of \
                  reach or close to a singularity'.format(pose.Pos()))
            return False
        else:
            # If valid, set joint positions
            self.robot.setJoints( new_robot_joints )
            self.send_tcp( new_robot_joints )
            return True
 
    def set_gripper_state(self, new_state):
        """
        Set visual state of gripper.
        
        Current environment has states opened, half-closed and fully-closed, going from index 0 to 2 respectively.
        
        @params
        new_state : int
            Index of state within [0, 1, 2] for opened, half-closed and fully closed respectively.

        """     
        if new_state not in [0, 1, 2]:
            print('[SET_GRIPPER_STATE] Gripper state not existent: {}'.format(new_state))
            
        if new_state == self.gripper_state:
            return
 
        current_gripper =  self.gripper_models[new_state]
 
        if new_state: # If closing...
            for item in self.handy_parts:
                if item.Collision(current_gripper):  # And colliding...
                   item.setParentStatic(current_gripper)  # Attach
                   break
        else:         # If opening...
            self.gripper_models[self.gripper_state].DetachAll()
 
        gripper_models = self.gripper_models
        current_gripper.setVisible(True)
        for state, model in enumerate(gripper_models):
            if state == new_state:
                continue
            model.setVisible(False)
 
        # Update state
        self.gripper_state = new_state
  
    def test_points(self, filename):
        """
        Test function. Loads XYZ points from filename.csv, then apply self.transformation to it.
        Then, move to resulting point relatively to self.start_frame.
        
        @params
        filename : str
            name of .CSV without header containing XYZ points.
        """
        # Open file and split lines
        f = open(filename, 'r')
        reader = csv.reader(f, delimiter=',')
        # Read each line
        for row in reader:
           # Split components
           x, y, z = [float(element) for element in row[0].split()]
           
           """ Transform
           x_b = -z
           y_b = -x
           z_b = -y """
           point = [ -z, -x, -y]
           
           # Move robot to that position
           self.set_p_rel_start(self.transform_via_matrix(point))
           print('[MOVING] {}'.format(point))
           
        f.close()
                
#if __name__ == '__main__':
#    robotObject = RobotArm()
#    robotObject.set_map_transform([210, 210, 210], [0, 0, 0] )
#    robotObject.test_points('../data/data_02_xyz_only.txt')
