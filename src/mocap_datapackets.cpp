#include "mocap_optitrack/mocap_datapackets.h"

#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>
#include <ros/console.h>
using namespace std;

const geometry_msgs::PointStamped Marker::get_ros_point()
{
  geometry_msgs::PointStamped ros_point;
  ros_point.header.stamp = ros::Time::now();
  ros_point.point = get_3d_point();
  return ros_point;
}

const geometry_msgs::Point Marker::get_3d_point()
{
  // Note this transforms the point from Optitrack frame to map frame
  geometry_msgs::Point point;
  point.x = -x;
  point.y = z;
  point.z = y;

  // double _x=z;
  // double _y=x;
  // double _z=y;

  // comment next 3 lines to rest the x y z, or use new vale to update it.
  // point.x=9.982049844045033371e-01*_x-1.160667134821164598e-02*_y+4.073393585425559571e-02*_z-1.460863404576608460e+00;
  // point.y=2.705920774061649006e-02*_x+9.999020377956358008e-01*_y+4.895035610760244127e-02*_z-5.968177969212870027e-01;
  // point.z=-5.342853532014090606e-02*_x-7.823042397284655364e-03*_y+9.979702445998881455e-01*_z+4.396596820466142780e-02;

  // 0.98007726 -0.0160594 0.01042158 -1.43607409
  // 0.03189566 0.97930732 -0.01221701 -0.58540206
  // -0.03742537 0.0039356 0.98327934 0.01527554
 //  9.982049844045033371e-01,-1.160667134821164598e-02,4.073393585425559571e-02,-1.460863404576608460e+00
 //  2.705920774061649006e-02,9.999020377956358008e-01,4.895035610760244127e-02,-5.968177969212870027e-01
 // -5.342853532014090606e-02,-7.823042397284655364e-03,9.979702445998881455e-01,4.396596820466142780e-02


  // The transformation matrix comes from calibration
  const vector<double> T = {};

  // TODO
  return point;
}


RigidBody::RigidBody()
  : NumberOfMarkers(0), marker(0)
{
}

RigidBody::~RigidBody()
{
  delete[] marker;
}

const geometry_msgs::PoseStamped RigidBody::get_ros_pose(bool newCoordinates)
{
  geometry_msgs::PoseStamped ros_pose;
  ros_pose.header.stamp = ros::Time::now();

  if (newCoordinates)
  {
    // Motive 1.7+ coordinate system
    ros_pose.pose.position.x = -pose.position.x;
    ros_pose.pose.position.y = pose.position.z;
    ros_pose.pose.position.z = pose.position.y;

    ros_pose.pose.orientation.x = -pose.orientation.x;
    ros_pose.pose.orientation.y = pose.orientation.z;
    ros_pose.pose.orientation.z = pose.orientation.y;
    ros_pose.pose.orientation.w = pose.orientation.w;
  }
  else
  {
    // y & z axes are swapped in the Optitrack coordinate system
    // We DON't transform the pose, relying on ROS tf tree.
    ros_pose.pose.position.x = pose.position.x;
    ros_pose.pose.position.y = pose.position.y;
    ros_pose.pose.position.z = pose.position.z;

    ros_pose.pose.orientation.x = pose.orientation.x;
    ros_pose.pose.orientation.y = pose.orientation.y;
    ros_pose.pose.orientation.z = pose.orientation.z;
    ros_pose.pose.orientation.w = pose.orientation.w;
  }
  return ros_pose;
}

bool RigidBody::has_data()
{
    static const char zero[sizeof(pose)] = { 0 };
    return memcmp(zero, (char*) &pose, sizeof(pose));
}

ModelDescription::ModelDescription()
  : numMarkers(0), markerNames(0)
{
}

ModelDescription::~ModelDescription()
{
  delete[] markerNames;
}

ModelFrame::ModelFrame()
  : markerSets(0), otherMarkers(0), rigidBodies(0),
    numMarkerSets(0), numOtherMarkers(0), numRigidBodies(0),
    latency(0.0)
{
}

ModelFrame::~ModelFrame()
{
  delete[] markerSets;
  delete[] otherMarkers;
  delete[] rigidBodies;
}

Version::Version()
  : v_major(0), v_minor(0), v_revision(0), v_build(0)
{
}

Version::Version(int major, int minor, int revision, int build)
  : v_major(major), v_minor(minor), v_revision(revision), v_build(build)
{
  std::ostringstream ostr;
  ostr << v_major << "." << v_minor << "." << v_revision << "." << v_build;
  v_string  = ostr.str();
}

Version::Version(const std::string& version)
  : v_string(version)
{
  std::sscanf(version.c_str(), "%d.%d.%d.%d", &v_major, &v_minor, &v_revision, &v_build);
}

Version::~Version()
{
}
void Version::setVersion(int major, int minor, int revision, int build)
{
  v_major = major;
  v_minor = minor;
  v_revision = revision;
  v_build = build;

}

const std::string& Version::getVersionString()
{
  return this->v_string;
}

bool Version::operator > (const Version& comparison)
{
  if (v_major > comparison.v_major)
    return true;
  if (v_minor > comparison.v_minor)
    return true;
  if (v_revision > comparison.v_revision)
    return true;
  if (v_build > comparison.v_build)
    return true;
  return false;
}
bool Version::operator == (const Version& comparison)
{
  return v_major == comparison.v_major
      && v_minor == comparison.v_minor
      && v_revision == comparison.v_revision
      && v_build == comparison.v_build;
}

MoCapDataFormat::MoCapDataFormat(const char *packet, unsigned short length)
  : packet(packet), length(length), frameNumber(0)
{
}

MoCapDataFormat::~MoCapDataFormat()
{
}

void MoCapDataFormat::seek(size_t count)
{
  packet += count;
  length -= count;
}

void MoCapDataFormat::parse()
{
  seek(4); // skip 4-bytes. Header and size.

  // parse frame number
  read_and_seek(frameNumber);
  ROS_DEBUG("Frame number: %d", frameNumber);

  // count number of packetsets
  read_and_seek(model.numMarkerSets);
  model.markerSets = new MarkerSet[model.numMarkerSets];
  ROS_DEBUG("Number of marker sets: %d", model.numMarkerSets);

  for (int i = 0; i < model.numMarkerSets; i++)
  {
    strcpy(model.markerSets[i].name, packet);
    seek(strlen(model.markerSets[i].name) + 1);

    ROS_DEBUG("Parsing marker set named: %s", model.markerSets[i].name);

    // read number of markers that belong to the model
    read_and_seek(model.markerSets[i].numMarkers);
    ROS_DEBUG("Number of markers in set: %d", model.markerSets[i].numMarkers);
    model.markerSets[i].markers = new Marker[model.markerSets[i].numMarkers];

    for (int k = 0; k < model.markerSets[i].numMarkers; k++)
    {
      // read marker positions
      read_and_seek(model.markerSets[i].markers[k]);
      float x = model.markerSets[i].markers[k].x;
      float y = model.markerSets[i].markers[k].y;
      float z = model.markerSets[i].markers[k].z;
      ROS_DEBUG("\t marker %d: [x=%3.2f,y=%3.2f,z=%3.2f]", k, x, y, z);
    }
  }

  // read number of 'other' markers. Unidentified markers. (cf. NatNet specs)
  read_and_seek(model.numOtherMarkers);
  model.otherMarkers = new Marker[model.numOtherMarkers];
  ROS_DEBUG("Number of markers not in sets: %d", model.numOtherMarkers);

  for (int l = 0; l < model.numOtherMarkers; l++)
  {
    // read positions of 'other' markers
    read_and_seek(model.otherMarkers[l]);
  }

  // read number of rigid bodies of the model
  read_and_seek(model.numRigidBodies);
  ROS_DEBUG("Number of rigid bodies: %d", model.numRigidBodies);

  model.rigidBodies = new RigidBody[model.numRigidBodies];
  for (int m = 0; m < model.numRigidBodies; m++)
  {
    // read id, position and orientation of each rigid body
    read_and_seek(model.rigidBodies[m].ID);
    read_and_seek(model.rigidBodies[m].pose);

    // get number of markers per rigid body
    read_and_seek(model.rigidBodies[m].NumberOfMarkers);

    ROS_DEBUG("Rigid body ID: %d", model.rigidBodies[m].ID);
    ROS_DEBUG("Number of rigid body markers: %d", model.rigidBodies[m].NumberOfMarkers);
    ROS_DEBUG("pos: [%3.2f,%3.2f,%3.2f], ori: [%3.2f,%3.2f,%3.2f,%3.2f]",
             model.rigidBodies[m].pose.position.x,
             model.rigidBodies[m].pose.position.y,
             model.rigidBodies[m].pose.position.z,
             model.rigidBodies[m].pose.orientation.x,
             model.rigidBodies[m].pose.orientation.y,
             model.rigidBodies[m].pose.orientation.z,
             model.rigidBodies[m].pose.orientation.w);

    if (model.rigidBodies[m].NumberOfMarkers > 0)
    {
      model.rigidBodies[m].marker = new Marker [model.rigidBodies[m].NumberOfMarkers];

      for (int _tmp = 0; _tmp < model.rigidBodies[m].NumberOfMarkers; ++_tmp)
      {
        read_and_seek(model.rigidBodies[m].marker[_tmp]);
        // ROS_DEBUG("marker %d: [%.2f, %.2f, %.2f]\n",
        //   _tmp, model.rigidBodies[m].marker[_tmp].x, model.rigidBodies[m].marker[_tmp].y, model.rigidBodies[m].marker[_tmp].z);
      }

      // skip marker
      // size_t byte_count = model.rigidBodies[m].NumberOfMarkers * sizeof(Marker);
      //memcpy(model.rigidBodies[m].marker, packet, byte_count);
      //seek(byte_count);

      // skip marker IDs
      size_t byte_count = model.rigidBodies[m].NumberOfMarkers * sizeof(int);
      seek(byte_count);
      //for (int _tmp = 0; _tmp < model.rigidBodies[m].NumberOfMarkers; ++_tmp)
      //{
      //  int _tmp_id;
      //  read_and_seek(_tmp_id);
      //  ROS_INFO("marker %d id %d", _tmp, _tmp_id);
      //}

      // skip marker sizes
      byte_count = model.rigidBodies[m].NumberOfMarkers * sizeof(float);
      seek(byte_count);
    }

    // skip mean marker error
    seek(sizeof(float));

    // 2.6 or later.
    if (NatNetVersion > Version("2.6"))
    {
      seek(sizeof(short));
    }

  }

  // TODO: read skeletons
  int numSkeletons = 0;
  read_and_seek(numSkeletons);

  // get latency
  read_and_seek(model.latency);
}
