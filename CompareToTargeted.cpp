//
// Created by Dennis Goldfarb on 11/14/16.
//
#include <iostream>
#include <fstream>
#include <string>

#include <OpenMS/FORMAT/MzMLFile.h>
#include <OpenMS/Math/MISC/MathFunctions.h>

#include "Ion.h"
#include "SpectrumUtilities.h"

void normalizeDist(std::vector<std::pair<double, double> > &dist)
{
    double maxProb = 0;
    for (auto itr = dist.begin(); itr != dist.end(); ++itr)
    {
        maxProb = std::max(maxProb, itr->second);
    }
    for (int i = 0; i < dist.size(); ++i)
    {
        dist[i].second /= maxProb;
    }
}

void outputDist(std::ofstream &out, std::vector<std::pair<double, double> > &dist, std::string ion_name, int ionIndex,
                std::string isotope_range, std::string name)
{


    for (int i = 0; i < dist.size(); ++i)
    {
        out << isotope_range << "\t" << ionIndex << "\t" << ion_name << "\t" << dist[i].first << "\t"
                 << dist[i].second << "\t" << name << std::endl;
    }
}

void usage()
{
    std::cout << "Usage: " << std::endl;
}

int main(int argc, char * argv[])
{

    OpenMS::MzMLFile mzMLDataFileProfile, mzMLDataFileCentroid;
    OpenMS::MSExperiment<OpenMS::Peak1D> msExperimentProfile, msExperimentCentroid;
    try {
        mzMLDataFileProfile.load(argv[1], msExperimentProfile);
        mzMLDataFileCentroid.load(argv[2], msExperimentCentroid);
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        usage();
        return 1;
    }

    std::ofstream out(argv[3]);
    std::ofstream calc_out(argv[4]);

    out << "isotope.range" << "\t" << "ion.index" << "\t" << "ion.name" << "\t" << "mz" << "\t" << "int" << std::endl;
    calc_out << "isotope.range" << "\t" << "ion.index" << "\t" << "ion.name" << "\t" << "mz" << "\t" << "int" << "\t" << "method" << std::endl;

    const Ion precursorIon = Ion(OpenMS::AASequence::fromString("[-18.010565]ELYENKPRRPYIL"), OpenMS::Residue::Full, 3);

    //create list of b and y ions
    std::vector<Ion> ionList;
    Ion::generateFragmentIons(ionList, precursorIon.sequence, precursorIon.charge);

    double isotopeStep = OpenMS::Constants::NEUTRON_MASS_U / precursorIon.charge;

    //Loop through all spectra
    //for (int specIndex = 0; specIndex < msExperimentCentroid.getNrSpectra(); ++specIndex) {
    for (int specIndex = 0; specIndex < 10; ++specIndex) {

        OpenMS::MSSpectrum<OpenMS::Peak1D> currentSpectrumCentroid = msExperimentCentroid.getSpectrum(specIndex);
        OpenMS::MSSpectrum<OpenMS::Peak1D> currentSpectrumProfile = msExperimentProfile.getSpectrum(specIndex);

        if (currentSpectrumCentroid.getMSLevel() == 1) continue;

        currentSpectrumCentroid.sortByPosition();

        const OpenMS::Precursor precursorInfo = currentSpectrumCentroid.getPrecursors()[0];
        std::vector<OpenMS::UInt> precursorIsotopes;

        //fill precursor isotopes vector
        SpectrumUtilities::whichPrecursorIsotopes(precursorIsotopes,
                               precursorInfo,
                               precursorIon,
                               0);

        //loop through each fragment ion
        for (int ionIndex = 0; ionIndex < ionList.size(); ++ionIndex) {
            //compute search peak matching tolerance
            double tol = OpenMS::Math::ppmToMass(20.0, ionList[ionIndex].monoMz);

            OpenMS::Int peakIndex = -1;
            for (int i = 0; i <= precursorIsotopes.back() && peakIndex == -1; ++i)
            {
                //find nearest peak to ion mz within tolerance
                peakIndex = currentSpectrumCentroid.findNearest(ionList[ionIndex].monoMz + (isotopeStep * i), tol);
            }

            // peak not found
            if (peakIndex == -1) continue;

            std::vector<std::pair<double, double> > exactPrecursorDist;
            std::vector<std::pair<double, double> > approxPrecursorFromWeightDist;
            std::vector<std::pair<double, double> > approxFragmentFromWeightDist;
            std::vector<std::pair<double, double> > approxFragmentFromWeightAndSulfurDist;
            std::vector<std::pair<double, double> > exactConditionalFragmentDist;
            std::vector<std::pair<double, double> > observedDist;

            SpectrumUtilities::exactConditionalFragmentIsotopeDist(exactConditionalFragmentDist,
                                                                   precursorIsotopes,
                                                                   ionList[ionIndex],
                                                                   precursorIon.sequence,
                                                                   precursorIon.charge);

            SpectrumUtilities::approxFragmentFromWeightIsotopeDist(approxFragmentFromWeightDist,
                                                precursorIsotopes,
                                                ionList[ionIndex],
                                                precursorIon.sequence,
                                                precursorIon.charge);

            SpectrumUtilities::approxFragmentFromWeightAndSIsotopeDist(approxFragmentFromWeightAndSulfurDist,
                                                    precursorIsotopes,
                                                    ionList[ionIndex],
                                                    precursorIon.sequence,
                                                    precursorIon.charge);



            //match theoretical distribution with observed peaks
            SpectrumUtilities::observedDistribution(observedDist, exactConditionalFragmentDist, currentSpectrumCentroid);
            //scale observed intensities across distribution
            SpectrumUtilities::scaleDistribution(observedDist);



            std::string isotope_range = std::to_string(precursorIsotopes.front());
            if (precursorIsotopes.size() > 1) isotope_range += "-" + std::to_string(precursorIsotopes.back());

            std::string ion_type = (ionList[ionIndex].type == OpenMS::Residue::ResidueType::BIon) ? "B" : "Y";
            ion_type += std::to_string(ionList[ionIndex].sequence.size());
            for (int i = 0; i < ionList[ionIndex].charge; ++i) ion_type += "+";
            std::string ion_name = ion_type + " " + ionList[ionIndex].sequence.toUnmodifiedString();




            float maxIntensity = 0;
            for (auto itr = currentSpectrumProfile.begin(); itr != currentSpectrumProfile.end(); ++itr)
            {
                if (itr->getMZ() >= observedDist.front().first - 0.5 &&
                    itr->getMZ() <= observedDist.back().first + 1)
                {
                    maxIntensity = std::max(maxIntensity, itr->getIntensity());
                }
            }


            out << isotope_range << "\t" << ionIndex << "\t" << ion_name << "\t" << ionList[ionIndex].monoMz - 0.5 << "\t" << 0 << std::endl;
            for (auto itr = currentSpectrumProfile.begin(); itr != currentSpectrumProfile.end(); ++itr)
            {
                if (itr->getMZ() >= observedDist.front().first - 0.5 &&
                        itr->getMZ() <= observedDist.back().first + 1)
                {
                    out << isotope_range << "\t" << ionIndex << "\t" << ion_name << "\t" << itr->getMZ() << "\t" << itr->getIntensity()/maxIntensity << std::endl;
                }
            }
            out << isotope_range << "\t" << ionIndex << "\t" << ion_name << "\t" << ionList[ionIndex].monoMz + 3.3 << "\t" << 0 << std::endl;


            normalizeDist(exactConditionalFragmentDist);
            normalizeDist(approxFragmentFromWeightDist);
            normalizeDist(approxFragmentFromWeightAndSulfurDist);

            outputDist(calc_out, exactConditionalFragmentDist, ion_name, ionIndex, isotope_range, "Exact Fragment");
            outputDist(calc_out, approxFragmentFromWeightDist, ion_name, ionIndex, isotope_range, "Approx Fragment");
            outputDist(calc_out, approxFragmentFromWeightAndSulfurDist, ion_name, ionIndex, isotope_range, "Approx Fragment S");
        }

    }

    return 0;
}