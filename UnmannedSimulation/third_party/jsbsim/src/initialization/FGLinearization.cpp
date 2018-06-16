/*
 * FGLinearization.cpp
 * Copyright (C) James Goppert 2011 <james.goppert@gmail.com>
 *
 * FGLinearization.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * FGLinearization.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FGInitialCondition.h"
#include "FGLinearization.h"
#include <ctime>

namespace JSBSim {

// TODO make FGLinearization have X,U,Y selectable by xml config file

FGLinearization::FGLinearization(FGFDMExec * fdm, int mode)
{
    std::cout << "\nlinearization: " << std::endl;
    std::clock_t time_start=clock(), time_linDone;
    FGStateSpace ss(fdm);

    ss.x.add(new FGStateSpace::Vt);
    ss.x.add(new FGStateSpace::Alpha);
    ss.x.add(new FGStateSpace::Theta);
    ss.x.add(new FGStateSpace::Q);

    // get propulsion pointer to determine type/ etc.
    FGEngine * engine0 = fdm->GetPropulsion()->GetEngine(0);
    FGThruster * thruster0 = engine0->GetThruster();

    if (thruster0->GetType()==FGThruster::ttPropeller)
    {
        ss.x.add(new FGStateSpace::Rpm0);
        // TODO add variable prop pitch property
        // if (variablePropPitch) ss.x.add(new FGStateSpace::PropPitch);
        int numEngines = fdm->GetPropulsion()->GetNumEngines();
        if (numEngines>1) ss.x.add(new FGStateSpace::Rpm1);
        if (numEngines>2) ss.x.add(new FGStateSpace::Rpm2);
        if (numEngines>3) ss.x.add(new FGStateSpace::Rpm3);
        if (numEngines>4) {
            std::cerr << "more than 4 engines not currently handled" << std::endl;
        }
    }
    ss.x.add(new FGStateSpace::Beta);
    ss.x.add(new FGStateSpace::Phi);
    ss.x.add(new FGStateSpace::P);
    ss.x.add(new FGStateSpace::Psi);
    ss.x.add(new FGStateSpace::R);
    ss.x.add(new FGStateSpace::Latitude);
    ss.x.add(new FGStateSpace::Longitude);
    ss.x.add(new FGStateSpace::Alt);

    ss.u.add(new FGStateSpace::ThrottleCmd);
    ss.u.add(new FGStateSpace::DaCmd);
    ss.u.add(new FGStateSpace::DeCmd);
    ss.u.add(new FGStateSpace::DrCmd);

    // state feedback
    ss.y = ss.x;

    std::vector< std::vector<double> > A,B,C,D;
    std::vector<double> x0 = ss.x.get(), u0 = ss.u.get();
    std::vector<double> y0 = x0; // state feedback
    std::cout << ss << std::endl;

    ss.linearize(x0,u0,y0,A,B,C,D);

    int width=10;
    std::cout.precision(3);
    std::cout
        << std::fixed
        << std::right
        << "\nA=\n" << std::setw(width) << A
        << "\nB=\n" << std::setw(width) << B
        << "\nC=\n" << std::setw(width) << C
        << "\n* note: C should be identity, if not, indicates problem with model"
        << "\nD=\n" << std::setw(width) << D
        << std::endl;

    // write scicoslab file
    std::string aircraft = fdm->GetAircraft()->GetAircraftName();
    std::ofstream scicos(std::string(aircraft+"_lin.sce").c_str());
    scicos.precision(10);
    width=20;
    scicos
    << std::scientific
    << aircraft << ".x0=..\n" << std::setw(width) << x0 << ";\n"
    << aircraft << ".u0=..\n" << std::setw(width) << u0 << ";\n"
    << aircraft << ".sys = syslin('c',..\n"
    << std::setw(width) << A << ",..\n"
    << std::setw(width) << B << ",..\n"
    << std::setw(width) << C << ",..\n"
    << std::setw(width) << D << ");\n"
    << aircraft << ".tfm = ss2tf(" << aircraft << ".sys);\n"
    << std::endl;

    time_linDone = std::clock();
    std::cout << "\nlinearization computation time: " << (time_linDone - time_start)/double(CLOCKS_PER_SEC) << " s\n" << std::endl;
}


} // JSBSim

// vim:ts=4:sw=4
