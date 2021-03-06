/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 1991-2015 ESI-OpenCFD Ltd.
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is a derivative work of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.


Application
    cloggingFoam
    Ismail El Mellas
Description
    Steady solver for incompressible, suspensions flow of particles.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "pimpleControl.H"
#include "CorrectPhi.H"
#include "fvOptions.H"
#include "subCycle.H"
//#include "interpolationTable.H"
#include "pimpleControl.H"
#include "turbulentTransportModel.H"
#include "singlePhaseTransportModel.H"
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
#   include "postProcess.H"
#   include "setRootCaseLists.H"
#   include "createTime.H"
#   include "createTimeControls.H"
#   include "createMesh.H"
#   include "initContinuityErrs.H"
#   include "createFields.H"
#   include "createUfIfPresent.H"
#   include "CourantNo.H"
#   include "setInitialDeltaT.H"

    pimpleControl pimple(mesh);
    turbulence->validate();


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info<< "\nStarting time loop\n" << endl;

    while (runTime.run())
    {

#       include "CourantNo.H"
#       include "setDeltaT.H"

        runTime++;

        Info<< "Time = " << runTime.timeName() << nl << endl;


        // --- Pressure-velocity PIMPLE corrector loop
        while (pimple.loop()) {

            // Einstein relation
            nu = turbulence->nu() * pow( (1.0 - (psi/psim)), -2);
            // ageing Viscosity
            //nu = nu * (scalar(1.0)+pow(lamda, 1.0)*psi);
            rho = rho0-as*(T - Ti);


            gSum(mesh.V());
            gSum(psi.primitiveField()*mesh.V());
            Info << "Solving for mixture velocity" << endl;
            #include "UEqn.H"

            while(pimple.correct())
            {
                Info << "Solving for mixture pressure" << endl;
                #include "pEqn.H"

                Info << "Solving for dispersed phase slip" << endl;
                #include "updatePhiSlip.H"
                //#include "updatePhiSlipRegularised.H"
            }

            const volScalarField gamma
              ("gamma",
               Foam::sqrt(scalar(2))*mag(symm(fvc::grad(U)))
              );

            const surfaceScalarField phipsi
              ("phi",
                phi+phislip
              );

            const volScalarField w
              ("w",
                scalar(1)-psi
              );

            const volScalarField psifact
                ("psifact",
                  MM*(1/rho)
                );

            const surfaceScalarField phic
              ("phi",
                phi*fvc::interpolate(w)
              );

            const volScalarField DT
                ("DT",
                  k*(1/rho)*(1/cp)
                );


            Info << "Solving for temperature" << endl;
            #include "TEqn.H"

            Info << "Computing source terms" << endl;
            #include "SEqn.H"

            Info << "Solving for dispersed phase" << endl;
            #include "psiEqn.H"

            Info << "Solving for concentration" << endl;
            #include "cEqn.H"

            Info << "Solving for viscosity" << endl;
            //#include "lamdaEqn.H"

            laminarTransport.correct();
            turbulence->correct();
        }

        runTime.write();
        Info<<"fluxOut= " << gSum(psi.primitiveField()*mesh.V()) << nl << endl;
        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
    }

    Info<< "End\n" << endl;

    return(0);
}


// ************************************************************************* //
