// Aircraft.cpp -- Implements a Aeromatic Aircraft type.
//
// Based on Aeromatic2 PHP code by David P. Culp
// Started June 2003
//
// C++-ified and modulized by Erik Hofman, started October 2015.
//
// Copyright (C) 2003, David P. Culp <davidculp2@comcast.net>
// Copyright (C) 2015 Erik Hofman <erik@ehofman.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <math.h>
#include <time.h>

#include <locale>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <Systems/Systems.h>
#include "Aircraft.h"
#include "types.h"

namespace Aeromatic
{

Aircraft::Aircraft(Aeromatic *p = 0) :
    _subtype(0),
    _overwrite(true),
    _subdir(true),
    _engines(0),
    _aircraft(p)
{
                    /* general information */
#ifdef WIN32
    std::string dir(getEnv("HOMEPATH"));
#else
    std::string dir(getEnv("HOME"));
#endif
    strCopy(_path, dir);
    _general.push_back(new Param("Output directory", "Specify the output directory for the configuration files", _path));

    _general.push_back(new Param("Create a subdirectory?", "Set to yes to create a new subdirectory with the same name as the aircraft", _subdir));

    _general.push_back(new Param("Overwrite?", "Overwrite files that are already present?", _overwrite));

    strCopy(_name, "my_aircraft");
    _general.push_back(new Param("Aircraft name", "This defines the name and filename of the aircraft", _name));
}

Aircraft::~Aircraft()
{
}

Aeromatic::Aeromatic() : Aircraft(),
    _atype(LIGHT),
    _system_files(true),
    _metric(0),
    _stall_speed(0),
    _max_weight(10000.0f),
    _empty_weight(0),
    _length(40.0f),
    _wing_shape(STRAIGHT),
    _wing_span(40.0f),
    _wing_area(0),
    _wing_chord(0),
    _wing_incidence(0),
    _wing_dihedral(0),
    _wing_sweep(0),
    _htail_area(0),
    _htail_arm(0),
    _vtail_area(0),
    _vtail_arm(0),
    _aspect_ratio(0),
    _taper_ratio(0),
    _payload(10000.0f),
    _no_engines(0)
{
    _inertia[0] = _inertia[1] = _inertia[2] = 0.0;
    _payload = _max_weight;

    /* general information */
    _general.push_back(new Param("Use dedicates System files?", "Select no to keep all systems in the aircraft configuration file", _system_files));
    Param* units = new Param("Select a system of measurement", "The options affects all units for length, surface area, speed and thrust/power", _metric);
    _general.push_back(units);
    units->add_option("English (feet, pounds)");
    units->add_option("Metric (meters, kilograms)");

    /* performance, weight and balance */
    _weight_balance.push_back(new Param("Stall speed", "The stall speed (landing speed/1.3) halfway between max. weight and empty weight", _stall_speed, _metric, SPEED));
    _weight_balance.push_back(new Param("Maximum takeoff weight", 0, _max_weight, _metric, WEIGHT));
    _weight_balance.push_back(new Param("Empty weight", _estimate, _empty_weight, _metric, WEIGHT));
    _weight_balance.push_back(new Param("Inertia Ixx", _estimate, _inertia[X], _metric, INERTIA));
    _weight_balance.push_back(new Param("Inertia Iyy", _estimate, _inertia[Y], _metric, INERTIA));
    _weight_balance.push_back(new Param("Inertia Izz", _estimate, _inertia[Z], _metric, INERTIA));

    /* geometry */
    _geometry.push_back(new Param("Length", 0, _length, _metric, LENGTH));
    Param* wingshape = new Param("Select a wing shape", "Wing shapes determaine the lift and drag of the aircraft", _wing_shape);
    _geometry.push_back(wingshape);
    wingshape->add_option("Straight");
    wingshape->add_option("Elliptical");
    wingshape->add_option("Delta");
//  wingshape->add_option("Variable sweep");

    _geometry.push_back(new Param("Wing span", 0, _wing_span, _metric, LENGTH));
    _geometry.push_back(new Param("Wing area", _estimate, _wing_area, _metric, AREA));
    _geometry.push_back(new Param("Wing aspect ratio", _estimate, _aspect_ratio));
    _geometry.push_back(new Param("Wing taper ratio", _estimate, _taper_ratio));
    _geometry.push_back(new Param("Wing chord", _estimate, _wing_chord, _metric, LENGTH));
    _geometry.push_back(new Param("Wing incidence", _estimate, _wing_incidence));
    _geometry.push_back(new Param("Wing dihedral", _estimate, _wing_dihedral));
    _geometry.push_back(new Param("Wing sweep (max)", _estimate, _wing_sweep));
    _geometry.push_back(new Param("Htail area", _estimate, _htail_area, _metric, AREA));
    _geometry.push_back(new Param("Htail arm", _estimate, _htail_arm, _metric, LENGTH));
    _geometry.push_back(new Param("Vtail area", _estimate, _vtail_area, _metric, AREA));
    _geometry.push_back(new Param("Vtail arm", _estimate, _vtail_arm, _metric, LENGTH));

    Param *param = new Param("Type of aircraft", "Select closest aerodynamic type", _atype);
    _general.push_back(param);

    _aircraft[0] = new Light(this);
    param->add_option(_aircraft[0]->get_description());
    _aircraft[1] = new Performance(this);
    param->add_option(_aircraft[1]->get_description());
    _aircraft[2] = new Fighter(this);
    param->add_option(_aircraft[2]->get_description());
    _aircraft[3] = new JetTransport(this);
    param->add_option(_aircraft[3]->get_description());
    _aircraft[4] = new PropTransport(this);
    param->add_option(_aircraft[4]->get_description());

    Aircraft::_aircraft = this;
}

Aeromatic::~Aeromatic()
{
    for (unsigned i=0; i<MAX_AIRCRAFT; ++i) {
        delete _aircraft[i];
    }

    std::vector<Param*>::iterator it;
    for(it = _general.begin(); it != _general.end(); ++it) {
        delete *it;
    }
    for(it = _weight_balance.begin(); it != _weight_balance.end(); ++it) {
        delete *it;
    }
    for(it = _geometry.begin(); it != _geometry.end(); ++it) {
        delete *it;
    }
}

bool Aeromatic::fdm()
{
    Aircraft *aircraft = _aircraft[_atype];
    std::vector<System*> systems = _aircraft[_atype]->get_systems();

    _engines = _MIN(_no_engines, 4);
    aircraft->_engines = _engines;


//***** METRICS ***************************************
    _payload = _max_weight;

    // first, estimate wing loading in psf
    float wing_loading = aircraft->get_wing_loading();

    // if no wing area given, use wing loading to estimate
    bool wingarea_input;
    if (_wing_area == 0)
    {
        wingarea_input = false;
        _wing_area = _max_weight / wing_loading;
    }
    else
    {
        wingarea_input = true;
        wing_loading = _max_weight / _wing_area;
    }

    // calculate wing chord
    if (_aspect_ratio == 0) {
        _aspect_ratio = aircraft->get_aspect_ratio();
    }
    if (_wing_chord == 0)
    {
        if (_aspect_ratio > 0) {
            _wing_chord = _wing_span / _aspect_ratio;
        } else {
            _wing_chord = _wing_area / _wing_span;
        }
    }

    // calculate aspect ratio
    if (_aspect_ratio == 0) {
        _aspect_ratio = (_wing_span*_wing_span) / _wing_area;
    }

    if (_taper_ratio == 0) {
        _taper_ratio = 1.0f;
    }

    // for now let's use a standard 2 degrees wing incidence
    if (_wing_incidence == 0) {
        _wing_incidence = 2.0;
    }

    // estimate horizontal tail area
    _htail_area = _wing_area * aircraft->get_htail_area();

    // estimate distance from CG to horizontal tail aero center
    _htail_arm = _length * aircraft->get_htail_arm();

    // estimate vertical tail area
    _vtail_area = _wing_area * aircraft->get_vtail_area();

    // estimate distance from CG to vertical tail aero center
    _vtail_arm = _length * aircraft->get_vtail_arm();

//***** EMPTY WEIGHT *********************************

    // estimate empty weight, based on max weight
    _empty_weight = _max_weight * aircraft->get_empty_weight();


//***** MOMENTS OF INERTIA ******************************

    // use Roskam's formulae to estimate moments of inertia
    if (_inertia[X] == 0.0f && _inertia[Y] == 0.0f && _inertia[Z] == 0.0f)
    {
        float slugs = (_empty_weight / 32.2f);	// sluggishness
        const float *R = aircraft->get_roskam();

        // These are for an empty airplane
        _inertia[X] = slugs * powf((R[X] * _wing_span / 2), 2);
        _inertia[Y] = slugs * powf((R[Y] * _length / 2), 2);
        _inertia[Z] = slugs * powf((R[Z] * ((_wing_span + _length)/2)/2), 2);
    }

//***** CG LOCATION ***********************************

    float cg_loc[3];
    cg_loc[X] = (_length - _htail_arm) * FEET_TO_INCH;
    cg_loc[Y] = 0;
    cg_loc[Z] = -(_length / 40.0f) * FEET_TO_INCH;

//***** AERO REFERENCE POINT **************************

    float aero_rp[3];
    aero_rp[X] = cg_loc[X];
    aero_rp[Y] = 0;
    aero_rp[Z] = 0;

//***** PILOT EYEPOINT *********************************

    // place pilot's eyepoint based on airplane type
    const float *_eyept_loc = aircraft->get_eyept_loc();
    float eyept_loc[3];
    eyept_loc[X] = (_length * _eyept_loc[X]) * FEET_TO_INCH;
    eyept_loc[Y] = _eyept_loc[Y];
    eyept_loc[Z] = _eyept_loc[Z];

//***** PAYLOAD ***************************************

    // A point mass will be placed at the CG weighing
    // 1/2 of the usable aircraft load.
    float payload_loc[3];
    payload_loc[X] = cg_loc[X];
    payload_loc[Y] = cg_loc[Y];
    payload_loc[Z] = cg_loc[Z];
    _payload -= _empty_weight;

//***** SYSTEMS ***************************************
    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled()) {
            systems[i]->set(cg_loc);
        }
    }

//***** COEFFICIENTS **********************************
    aircraft->set_lift();
    aircraft->set_drag();
    aircraft->set_side();
    aircraft->set_roll();
    aircraft->set_pitch();
    aircraft->set_yaw();

//************************************************
//*                                              *
//*  Print out xml document                      *
//*                                              *
//************************************************

    char str[64];
    time_t t;

    time(&t);
#ifdef _MSC_VER
    struct tm ti;
    localtime_s(&ti, &t);
    strftime(str, sizeof(str), "%d %b %Y", &ti);
#else
    struct tm *ti= localtime(&t);
    strftime(str, sizeof(str), "%d %b %Y", ti);
#endif

    _dir = _subdir ? create_dir(_path, _name) : _path;
    if (_dir.empty()) {
        std::cout << "Unable to create directory: " << _path << "/" << _name << std::endl;
        return false;
    }

    std::string systems_dir;
    if (_system_files)
    {
        systems_dir = create_dir(_dir, "Systems");
        if (systems_dir.empty())
        {
            std::cout << "Unable to create directory: " << _dir<< "/Systems" << std::endl;
            _system_files = false;
        }
    }

    std::string fname = _dir + "/" + std::string(_name) + ".xml";

    std::string version = AEROMATIC_VERSION_STR;

    if (!_overwrite && overwrite(fname)) {
        std::cout << "File already exists: " << fname << std::endl;
        return false;
    }

    std::ofstream file;
    file.open(fname.c_str());
    if (file.fail() || file.bad())
    {
        file.close();
        return false;
    }

    file.precision(2);
    file.flags(std::ios::right);
    file << std::fixed << std::showpoint;

    file << "<?xml version=\"1.0\"?>" << std::endl;
    file << "<?xml-stylesheet type=\"text/xsl\" href=\"http://jsbsim.sourceforge.net/JSBSim.xsl\"?>" << std::endl;
    file << std::endl;
    file << "<fdm_config name=\"" << _name << "\" version=\"2.0\" release=\"ALPHA\"" << std::endl;
    file << "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
    file << "   xsi:noNamespaceSchemaLocation=\"http://jsbsim.sourceforge.net/JSBSim.xsd\">" << std::endl;
    file << std::endl;
    file << " <fileheader>" << std::endl;
    file << "  <author> Aeromatic v " << version << " </author>" << std::endl;
    file << "  <filecreationdate> " << str << " </filecreationdate>" << std::endl;
    file << "  <version>$Revision: 1.31 $</version>" << std::endl;
    file << "  <description> Models a " << _name << ". </description>" << std::endl;
    file << " </fileheader>" << std::endl;
    file << std::endl;
    file << "<!--\n  File:     " << _name << ".xml" << std::endl;
    file << "  Inputs:" << std::endl;
    file << "    name:          " << _name << std::endl;
    file << "    type:          ";
    switch(_atype)
    {
    case LIGHT:
        if (_no_engines == 0) {
            file << "glider" << std::endl;
        } else {
            file << "light commuter with " << _no_engines << " engines" << std::endl;
        }
        break;
    case PERFORMANCE:
        file << "WWII fighter, subsonic sport, aerobatic" << std::endl;
        break;
    case FIGHTER:
        file << _no_engines << " engine transonic/supersonic fighter" << std::endl;
        break;
    case JET_TRANSPORT:
        file << _no_engines << " engine transonic transport" << std::endl;
        break;
    case PROP_TRANSPORT:
        file << "multi-engine prop transport" << std::endl;
        break;
    }
    file << "    max weight:    " << _max_weight << " lb" << std::endl;
    file << "    wing span:     " << _wing_span << " ft" << std::endl;
    file << "    length:        " << _length << " ft" << std::endl;
    file << "    wing area:     ";
    if (wingarea_input) {
        file << _wing_area << " sq-ft" << std::endl;
    } else {
        file << "unspecified" << std::endl;
    }
    file << "    aspect ratio:  " << _aspect_ratio << ":1" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled()) {
            std::string comment = systems[i]->comment();
            if (!comment.empty()) {
                file << comment << std::endl;
            }
        }
    }

    file << "  Outputs:" << std::endl;
    file << "    wing loading:  " << wing_loading << " lb/sq-ft" << std::endl;
    file << "    payload:       " << _payload << " lbs" << std::endl;
    file << "    CL-alpha:      " << _CLalpha[0] << " per radian" << std::endl;
    file << "    CL-0:          " << _CL0 << std::endl;
    file << "    CL-max:        " << _CLmax[0] << std::endl;
    file << "    CD-0:          " << _CD0 << std::endl;
    file << "    K:             " << _K << std::endl;
    file << "-->" << std::endl;
    file << std::endl;

//***** METRICS **********************************

    file << " <metrics>" << std::endl;
    file << "   <wingarea  unit=\"FT2\"> " << std::setw(8) << _wing_area << " </wingarea>" << std::endl;
    file << "   <wingspan  unit=\"FT\" > " << std::setw(8) << _wing_span << " </wingspan>" << std::endl;
    file << "   <wing_incidence>       " << std::setw(8) << _wing_incidence << " </wing_incidence>" << std::endl;
    file << "   <chord     unit=\"FT\" > " << std::setw(8) << _wing_chord << " </chord>" << std::endl;
    file << "   <htailarea unit=\"FT2\"> " << std::setw(8) << _htail_area << " </htailarea>" << std::endl;
    file << "   <htailarm  unit=\"FT\" > " << std::setw(8) << _htail_arm << " </htailarm>" << std::endl;
    file << "   <vtailarea  unit=\"FT2\">" << std::setw(8) << _vtail_area << " </vtailarea>" << std::endl;
    file << "   <vtailarm  unit=\"FT\" > " << std::setw(8) << _vtail_arm << " </vtailarm>" << std::endl;
    file << "   <location name=\"AERORP\" unit=\"IN\">" << std::endl;
    file << "     <x> " << std::setw(8) << aero_rp[X] << " </x>" << std::endl;
    file << "     <y> " << std::setw(8) << aero_rp[Y] << " </y>" << std::endl;
    file << "     <z> " << std::setw(8) << aero_rp[Z] << " </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "   <location name=\"EYEPOINT\" unit=\"IN\">" << std::endl;
    file << "     <x> " << std::setw(8) << eyept_loc[X] << " </x>" << std::endl;
    file << "     <y> " << std::setw(8) << eyept_loc[Y] << " </y>" << std::endl;
    file << "     <z> " << std::setw(8) << eyept_loc[Z] << " </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "   <location name=\"VRP\" unit=\"IN\">" << std::endl;
    file << "     <x>     0.0 </x>" << std::endl;
    file << "     <y>     0.0 </y>" << std::endl;
    file << "     <z>     0.0 </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << " </metrics>"<< std::endl;
    file << std::endl;
    file << " <mass_balance>" << std::endl;
    file << "   <ixx unit=\"SLUG*FT2\">  " << std::setw(8) << _inertia[X] << " </ixx>" << std::endl;
    file << "   <iyy unit=\"SLUG*FT2\">  " << std::setw(8) << _inertia[Y] << " </iyy>" << std::endl;
    file << "   <izz unit=\"SLUG*FT2\">  " << std::setw(8) << _inertia[Z] << " </izz>" << std::endl;
    file << "   <emptywt unit=\"LBS\" >  " << std::setw(8) << _empty_weight << " </emptywt>" << std::endl;
    file << "   <location name=\"CG\" unit=\"IN\">" << std::endl;
    file << "     <x> " << std::setw(8) << cg_loc[X] << " </x>" << std::endl;
    file << "     <y> " << std::setw(8) << cg_loc[Y] << " </y>" << std::endl;
    file << "     <z> " << std::setw(8) << cg_loc[Z] << " </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "   <pointmass name=\"Payload\">" << std::endl;
    file << "    <description> " << _payload << " LBS should bring model up to entered max weight </description>" << std::endl;
    file << "    <weight unit=\"LBS\"> " << (_payload* 0.5f) << " </weight>" << std::endl;
    file << "    <location name=\"POINTMASS\" unit=\"IN\">" << std::endl;
    file << "     <x> " << std::setw(8) << payload_loc[X] << " </x>" << std::endl;
    file << "     <y> " << std::setw(8) << payload_loc[Y] << " </y>" << std::endl;
    file << "     <z> " << std::setw(8) << payload_loc[Z] << " </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "  </pointmass>" << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string mass_balance = systems[i]->mass_balance();
            if (!mass_balance.empty()) {
                file << mass_balance << std::endl;
            }
        }
    }

    file << " </mass_balance>" << std::endl;
    file << std::endl;

//***** FDM_CONFIG ********************************************

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string fdm = systems[i]->fdm();
            if (!fdm.empty()) {
                file << fdm << std::endl;
            }
        }
    }

//***** SYSTEMS ***********************************************

    if (_system_files == true)
    {
        for (unsigned i=0; i<systems.size(); ++i)
        {
            if (systems[i]->enabled())
            {
                std::string system = systems[i]->system();
                if (!system.empty())
                {
                    std::string sname = systems[i]->get_description();
                    std::string sfname = sname + ".xml";

                    if (!_overwrite && overwrite(sfname))
                    {
                        std::cout << "File already exists: " << fname << std::endl;
                        std::cout << "Skipping." << std::endl;
                    }
                    else
                    {
                        file << " <system file=\"" << sfname << "\"/>" << std::endl;

                        std::string sfpath = systems_dir + "/" + sfname;
                        std::ofstream sfile;
                        sfile.open(sfpath.c_str());
                        if (sfile.fail() || sfile.bad())
                        {
                            std::cout << "Error opening file: " << fname << std::endl;
                            std::cout << "Skipping." << std::endl;
                        }
                        else
                        {
                            sfile << "<?xml version=\"1.0\"?>" << std::endl;
                            sfile << "<system name=\"" << sname << "\">" << std::endl;
                            sfile << system << std::endl;
                            sfile << "</system>" << std::endl;
                        }
                        sfile.close();
                    }
                }
            }
        }
        file << std::endl;
    }

    file << " <flight_control name=\"FCS: " << _name << "\">" << std::endl;
    file << std::endl;

    if (_system_files == false)
    {
        for (unsigned i=0; i<systems.size(); ++i)
        {
            if (systems[i]->enabled())
            {
                std::string system = systems[i]->system();
                if (!system.empty()) {
                    file << system << std::endl;
                }
            }
        }
    }

    file << " </flight_control>"<< std::endl;
    file << std::endl;

//***** AERODYNAMICS ******************************************

    file << " <aerodynamics>" << std::endl;
    file << std::endl;

    // ***** LIFT ******************************************

    file << "  <axis name=\"LIFT\">" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string lift = systems[i]->lift();
            if (!lift.empty()) {
                file << lift << std::endl;
            }
        }
    }

    file << "  </axis>" << std::endl;
    file << std::endl;

    // ***** DRAG ******************************************

    file << "  <axis name=\"DRAG\">" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string drag = systems[i]->drag();
            if (!drag.empty()) {
               file << drag << std::endl;
            }
        }
    }

    file << "  </axis>" << std::endl;
    file << std::endl;

    // ***** SIDE ******************************************

    file << "  <axis name=\"SIDE\">" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string side = systems[i]->side();
            if (!side.empty()) {
                file << side << std::endl;
            }
        }
    }

    file << "  </axis>" << std::endl;
    file << std::endl;

    // ***** PITCH *****************************************

    file << "  <axis name=\"PITCH\">" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string pitch = systems[i]->pitch();
            if (!pitch.empty()) {
                file << pitch << std::endl;
            }
        }
    }

    file << "  </axis>" << std::endl;
    file << std::endl;

    // ***** ROLL ******************************************

    file << "  <axis name=\"ROLL\">" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string roll = systems[i]->roll();
            if (!roll.empty()) {
                file << roll << std::endl;
            }
        }
    }

    file << "  </axis>" << std::endl;
    file << std::endl;

    // ***** YAW *******************************************

    file << "  <axis name=\"YAW\">" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string yaw = systems[i]->yaw();
            if (!yaw.empty()) {
                file << yaw << std::endl;
            }
        }
    }

    file << "  </axis>" << std::endl;
    file << std::endl;
    
    file << " </aerodynamics>" << std::endl;
    file << std::endl;

    file << " <external_reactions>" << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string force = systems[i]->external_force();
            if (!force.empty()) {
                file << force << std::endl;
            }
        }
    }

    file << " </external_reactions>" << std::endl;

    file << std::endl;
    file << "</fdm_config>" << std::endl;

    file.close();

    return true;
}

// ----------------------------------------------------------------------------

char const* Aeromatic::_estimate = "enter 0 to use estimated value";

#ifdef WIN32
#else
# include <sys/stat.h>
#endif

std::string Aeromatic::create_dir(std::string path, std::string subdir)
{
    // Create Engines directory
    std::string dir = path + "/" + subdir;
#ifdef WIN32
    if (!PathFileExists(dir.c_str())) {
        if (CreateDirectory(dir.c_str(), NULL) == 0) {
            dir.clear();
        }
    }
#else
    struct stat sb;
    if (stat(dir.c_str(), &sb))
    {
        int mode = strtol("0755", 0, 8);
        if (mkdir(dir.c_str(), mode) == -1) {
            dir.clear();
        }
    }
#endif

    return dir;
}

bool Aeromatic::overwrite(std::string path)
{
    bool rv = true;

#ifdef WIN32
    if (!PathFileExists(path.c_str())) {
        rv = false;
    }
#else
    struct stat sb;
    if (stat(path.c_str(), &sb) != 0) {
        rv = false;
    }
#endif

    return rv;
}

} /* namespace Aeromatic */

