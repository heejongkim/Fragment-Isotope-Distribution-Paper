//
// Created by Dennis Goldfarb on 11/17/16.
//

#ifndef FRAGMENT_ISOTOPE_DISTRIBUTION_PAPER_SPECTRUMUTILITIES_H
#define FRAGMENT_ISOTOPE_DISTRIBUTION_PAPER_SPECTRUMUTILITIES_H

#include <OpenMS/CHEMISTRY/IsotopeDistribution.h>
#include <OpenMS/CHEMISTRY/IsotopeSplineDB.h>
#include <OpenMS/CHEMISTRY/ElementDB.h>
#include <OpenMS/MATH/MISC/MathFunctions.h>

#include "Ion.h"



namespace SpectrumUtilities {

    static const double ERROR_PPM = 10;     //acquisition mz error for peak matching

    static const OpenMS::ElementDB* ELEMENTS = OpenMS::ElementDB::getInstance();   //element database

    static void normalizeDistribution(std::vector<std::pair<double, double> > &dist) {
        double sum = 0.0;
        for (std::pair<double, double> &peak : dist) {
            sum += peak.second;
        }
        for (std::pair<double, double> &peak : dist) {
            peak.second /= sum;
        }
    }

    std::set<OpenMS::UInt> whichPrecursorIsotopes(const OpenMS::Precursor &precursorInfo,
                                       const Ion &precursorIon, const double offset) {
        std::set<OpenMS::UInt> precursorIsotopes;
        //isolation window lower cutoff
        double lowerCutoff = precursorInfo.getMZ() - precursorInfo.getIsolationWindowLowerOffset() + offset;
        //isolation window upper cutoff
        double upperCutoff = precursorInfo.getMZ() + precursorInfo.getIsolationWindowUpperOffset() + offset;

        //distance between isotope peaks
        double isotopeStep = OpenMS::Constants::C13C12_MASSDIFF_U / precursorIon.charge;

        int smallestIsotope = std::max(0, int(std::ceil((lowerCutoff - precursorIon.monoMz) / isotopeStep)));
        int largestIsotope = int(std::floor((upperCutoff - precursorIon.monoMz) / isotopeStep));

        //loop through each isotope of precursor ion
        if (largestIsotope >= 0) {
            for (OpenMS::UInt m = smallestIsotope; m <= largestIsotope; ++m) {
                precursorIsotopes.insert(m);
            }
        }

        return precursorIsotopes;
    }

    /**
     * Identifies an isotope distribution within a mass spectrum based on the theoretical distribution mz values.
     * @param obsDist a vector to be filled with the observed isotope distribution. Composed of a vector of pairs
     * <double, double> the first being the mz of each isotope, the second being the intensity of the peak. An
     * intensity of 0 means the peak was not found in the spectrum. Vector will be cleared before being filled
     * with the distribution.
     * @param theoDist the theoretical isotopic distribution of which peaks will be searched.
     * @param spec the MS2 spectrum from which peaks will be located.
     */
    static void observedDistribution(std::vector<std::pair<double, double> > &obsDist,
                              std::vector<std::pair<double, double> > &theoDist,
                              const OpenMS::MSSpectrum<OpenMS::Peak1D> &spec)
    {
        //loop through each theoretical peak in isotopic distribution
        for (int i = 0; i < theoDist.size(); ++i) {
        //for (int i = 0; i < 7; ++i) {

            //calculate search tolerance
            double tol =  OpenMS::Math::ppmToMass(ERROR_PPM, theoDist[i].first);

            //find index of actual peak in spectrum
            OpenMS::Int isoPeakIndex = spec.findNearest(theoDist[i].first, tol);

            //observed isotope distribution pair
            std::pair<double, double> obs;

            if (isoPeakIndex == -1 && i < theoDist.size()) {
                //peak not found
                obs.first = theoDist[i].first;
                obs.second = 0;
                obsDist.push_back(obs);
            } else if (isoPeakIndex != -1) {
                //peak found
                obs.first = spec[isoPeakIndex].getMZ();
                obs.second = spec[isoPeakIndex].getIntensity();
                obsDist.push_back(obs);
            }
        }
        /*if (obsDist.size() > theoDist.size()) {
            std::cout << obsDist.size() << " " << theoDist.size() << std::endl;
        }*/
    }

    /**
     * Scales an isotopic distribution of peaks based on raw intensity to relative intensity which sum to 1 accross
     * all peaks in the distribution.
     * @param obsDist a vector of observed peaks within an isotopic distribution. Composed of a vector of pairs
     * <double, double> the first being the mz of each isotope, the second being the raw intensity of the peak.
     * Vector will be modified to contain scaled intensity values instead of raw intensity values.
     */
    std::vector<std::pair<double, double> > scaleDistribution(std::vector<std::pair<double, double> > &obsDist)
    {
        std::vector<std::pair<double, double> > scaled = obsDist;
        //sum intensities across distribution
        double totalIntensity = 0;
        for (int i = 0; i < scaled.size(); ++i) {
            totalIntensity += scaled[i].second;
        }

        //compute scaled intensity and replace value
        for (int j = 0; j < scaled.size(); ++j) {
            scaled[j].second = scaled[j].second / totalIntensity;
        }

        return scaled;
    }

    /**
     * Compute the exact theoretical fragment isotopic distribution based on the conditional fragment isotope distribution
     * calculator.
     * @param condDist a vector to be filled with the theoretical isotopic distribution. Composed of a vector of pairs
     * <double, double> the first being the mz of each isotope, the second the probability of seeing the peak (equivalent
                                                                                                              * to the peak abundance within the distribution). Vector will be cleared before being filled with distribution.
     * @param precursorIsotopes a vector representation of which precurosor isotopes were isolated within the ms2
     * isolation window. A vector <0, 1, 2> would represent the m0, m1, and m2 isotopes of an isotopic
     * distribution.
     * @param ion the Ion from which the monoisotopic peak will be based.
     * @param precursorSequence the amino acid sequence of the precursor peptide that was fragmented.
     * @param precursorCharge the charge of the precursor peptide that was fragmented.
     */
    static void exactConditionalFragmentIsotopeDist(std::vector<std::pair<double, double> > &condDist,
                                             const std::set<OpenMS::UInt> &precursorIsotopes,
                                             const Ion &ion,
                                             const OpenMS::AASequence &precursorSequence,
                                             const OpenMS::Int &precursorCharge)
    {
        //clear vector for distribution
        condDist.clear();

        //compute conditional isotopic distribution and get vector of isotope peaks
        OpenMS::EmpiricalFormula precursorFormula = precursorSequence.getFormula(OpenMS::Residue::Full, precursorCharge);
        std::vector<std::pair<OpenMS::Size, double> > condPeakList =
                ion.formula.getConditionalFragmentIsotopeDist(precursorFormula, precursorIsotopes).getContainer();

        //ion mz
        double ionMZ = ion.monoWeight / ion.charge;

        //loop through calculated isotopic distribution, fill with actual mz values
        for (int i = 0; i < condPeakList.size(); ++i) {

            //compute mz of isotope peak
            double isoMZ = ionMZ + ( OpenMS::Constants::C13C12_MASSDIFF_U / ion.charge ) * i;

            //set theoretical distribution pair
            std::pair<double, double> theo;
            theo.first = isoMZ;
            theo.second = condPeakList[i].second;
            condDist.push_back(theo);
        }
    }

    static void approxPrecursorFromWeightIsotopeDist(std::vector<std::pair<double, double> > &approxDist,
                                              const std::set<OpenMS::UInt> &precursorIsotopes,
                                              const Ion &fragmentIon)
    {
        OpenMS::UInt minIsotope = 7;
        //clear vector for distribution
        approxDist.clear();

        OpenMS::UInt maxIsotope = *std::max_element(precursorIsotopes.begin(), precursorIsotopes.end());
        //construct distribution of depth at the maximum precursor isotope isolated
        OpenMS::IsotopeDistribution fragmentDist(minIsotope);

        //estimate from fragment average weight
        fragmentDist.estimateFromPeptideWeight(fragmentIon.formula.getAverageWeight());
        //re-normalize distribution
        fragmentDist.renormalize();

        //get isotope vector
        std::vector<std::pair<OpenMS::Size, double> > isotopePeaks = fragmentDist.getContainer();

        //ion mz
        double ionMZ = fragmentIon.monoWeight / fragmentIon.charge;

        double basePeak = 0.0;
        for (int i = 0; i < isotopePeaks.size(); ++i) basePeak = std::max(basePeak, isotopePeaks[i].second);

        //loop through calculated isotopic distribution, fill with actual mz values
        for (int i = 0; i < isotopePeaks.size(); ++i) {

            if (isotopePeaks[i].second < basePeak * 0.1 && i > maxIsotope) continue;

            //compute mz of isotope peak
            double isoMZ = ionMZ + ( OpenMS::Constants::C13C12_MASSDIFF_U / fragmentIon.charge ) * i;

            //set distribution pair
            std::pair<double, double> theo;
            theo.first = isoMZ;
            theo.second = isotopePeaks[i].second;
            approxDist.push_back(theo);
        }

        normalizeDistribution(approxDist);

    }

    static void approxFragmentFromWeightIsotopeDist(std::vector<std::pair<double, double> > &approxDist,
                                             const std::set<OpenMS::UInt> &precursorIsotopes,
                                             const Ion &fragmentIon,
                                             const OpenMS::AASequence &precursorSequence,
                                             const OpenMS::Int &precursorCharge)
    {
        //clear vector for distribution
        approxDist.clear();

        //precursor average weight
        double precursorAvgWeight = precursorSequence.getAverageWeight(OpenMS::Residue::Full, precursorCharge);
        //fragment average weight
        double fragmentAvgWeight = fragmentIon.formula.getAverageWeight();

        //construct distribution
        OpenMS::IsotopeDistribution fragmentDist(*std::max_element(precursorIsotopes.begin(), precursorIsotopes.end()) + 1);

        //estimate approx distribution from peptide weight
        fragmentDist.estimateForFragmentFromPeptideWeight(precursorAvgWeight, fragmentAvgWeight, precursorIsotopes);
        //re-normalize distribution
        fragmentDist.renormalize();

        //get isotope vector
        std::vector<std::pair<OpenMS::Size, double> > isotopePeaks = fragmentDist.getContainer();

        //ion mz
        double ionMZ = fragmentIon.monoWeight / fragmentIon.charge;

        //loop through calculated isotopic distribution, fill with actual mz values
        for (int i = 0; i < isotopePeaks.size(); ++i) {

            //compute mz of isotope peak
            double isoMZ = ionMZ + ( OpenMS::Constants::C13C12_MASSDIFF_U / fragmentIon.charge ) * i;

            //set distribution pair
            std::pair<double, double> theo;
            theo.first = isoMZ;
            theo.second = isotopePeaks[i].second;
            approxDist.push_back(theo);
        }
    }

    static void approxFragmentFromWeightAndSIsotopeDist(std::vector<std::pair<double, double> > &approxDist,
                                                 const std::set<OpenMS::UInt> &precursorIsotopes,
                                                 const Ion &fragmentIon,
                                                 const OpenMS::AASequence &precursorSequence,
                                                 const OpenMS::Int &precursorCharge)
    {
        //clear vector for distribution
        approxDist.clear();

        //precursor average weight
        double precursorAvgWeight = precursorSequence.getAverageWeight(OpenMS::Residue::Full, precursorCharge);
        //precursor number of sulfurs
        int precursorSulfurs = precursorSequence.getFormula(OpenMS::Residue::Full,
                                                            precursorCharge).getNumberOf(ELEMENTS->getElement("Sulfur"));
        //fragment average weight
        double fragmentAvgWeight = fragmentIon.formula.getAverageWeight();
        //fragment number of sulfurs
        int fragmentSulfurs = fragmentIon.formula.getNumberOf(ELEMENTS->getElement("Sulfur"));

        //construct distribution
        OpenMS::IsotopeDistribution fragmentDist(*std::max_element(precursorIsotopes.begin(), precursorIsotopes.end()) + 1);

        //estimate approx distribution from peptide weight
        fragmentDist.estimateForFragmentFromPeptideWeightAndS(precursorAvgWeight, precursorSulfurs,
                                                              fragmentAvgWeight, fragmentSulfurs,
                                                              precursorIsotopes);
        //re-normalize distribution
        fragmentDist.renormalize();

        //get isotope vector
        std::vector<std::pair<OpenMS::Size, double> > isotopePeaks = fragmentDist.getContainer();

        //ion mz
        double ionMZ = fragmentIon.monoWeight / fragmentIon.charge;

        //loop through calculated isotopic distribution, fill with actual mz values
        for (int i = 0; i < isotopePeaks.size(); ++i) {

            //compute mz of isotope peak
            double isoMZ = ionMZ + ( OpenMS::Constants::C13C12_MASSDIFF_U / fragmentIon.charge ) * i;

            //set distribution pair
            std::pair<double, double> theo;
            theo.first = isoMZ;
            theo.second = isotopePeaks[i].second;
            approxDist.push_back(theo);
        }
    }

    static void approxFragmentSplineFromWeightIsotopeDist(std::vector<std::pair<double, double> > &approxDist,
                                                    const std::set<OpenMS::UInt> &precursorIsotopes,
                                                    const Ion &fragmentIon,
                                                    const OpenMS::AASequence &precursorSequence,
                                                    const OpenMS::Int &precursorCharge,
                                                    const OpenMS::IsotopeSplineDB* isotopeDB)
    {
        //clear vector for distribution
        approxDist.clear();

        //precursor average weight
        double precursorAvgWeight = precursorSequence.getAverageWeight(OpenMS::Residue::Full, precursorCharge);
        //fragment average weight
        double fragmentAvgWeight = fragmentIon.formula.getAverageWeight();

        //construct distribution
        OpenMS::IsotopeDistribution fragmentDist(*std::max_element(precursorIsotopes.begin(), precursorIsotopes.end()) + 1);

        //estimate approx distribution from peptide weight
        fragmentDist = isotopeDB->estimateForFragmentFromPeptideWeight(precursorAvgWeight, fragmentAvgWeight, precursorIsotopes);
        //re-normalize distribution
        fragmentDist.renormalize();

        //get isotope vector
        std::vector<std::pair<OpenMS::Size, double> > isotopePeaks = fragmentDist.getContainer();

        //ion mz
        double ionMZ = fragmentIon.monoWeight / fragmentIon.charge;

        //loop through calculated isotopic distribution, fill with actual mz values
        for (int i = 0; i < isotopePeaks.size(); ++i) {

            //compute mz of isotope peak
            double isoMZ = ionMZ + ( OpenMS::Constants::C13C12_MASSDIFF_U / fragmentIon.charge ) * i;

            //set distribution pair
            std::pair<double, double> theo;
            theo.first = isoMZ;
            theo.second = isotopePeaks[i].second;
            approxDist.push_back(theo);
        }
    }

    static void approxFragmentSplineFromWeightAndSIsotopeDist(std::vector<std::pair<double, double> > &approxDist,
                                                        const std::set<OpenMS::UInt> &precursorIsotopes,
                                                        const Ion &fragmentIon,
                                                        const OpenMS::AASequence &precursorSequence,
                                                        const OpenMS::Int &precursorCharge,
                                                        const OpenMS::IsotopeSplineDB* isotopeDB)
    {
        //clear vector for distribution
        approxDist.clear();

        //precursor average weight
        double precursorAvgWeight = precursorSequence.getAverageWeight(OpenMS::Residue::Full, precursorCharge);
        //precursor number of sulfurs
        int precursorSulfurs = precursorSequence.getFormula(OpenMS::Residue::Full,
                                                            precursorCharge).getNumberOf(ELEMENTS->getElement("Sulfur"));
        //fragment average weight
        double fragmentAvgWeight = fragmentIon.formula.getAverageWeight();
        //fragment number of sulfurs
        int fragmentSulfurs = fragmentIon.formula.getNumberOf(ELEMENTS->getElement("Sulfur"));

        //construct distribution
        OpenMS::IsotopeDistribution fragmentDist(*std::max_element(precursorIsotopes.begin(), precursorIsotopes.end()) + 1);

        //estimate approx distribution from peptide weight
        fragmentDist = isotopeDB->estimateForFragmentFromPeptideWeightAndS(precursorAvgWeight, precursorSulfurs,
                                                              fragmentAvgWeight, fragmentSulfurs,
                                                              precursorIsotopes);
        //re-normalize distribution
        fragmentDist.renormalize();

        //get isotope vector
        std::vector<std::pair<OpenMS::Size, double> > isotopePeaks = fragmentDist.getContainer();

        //ion mz
        double ionMZ = fragmentIon.monoWeight / fragmentIon.charge;

        //loop through calculated isotopic distribution, fill with actual mz values
        for (int i = 0; i < isotopePeaks.size(); ++i) {

            //compute mz of isotope peak
            double isoMZ = ionMZ + ( OpenMS::Constants::C13C12_MASSDIFF_U / fragmentIon.charge ) * i;

            //set distribution pair
            std::pair<double, double> theo;
            theo.first = isoMZ;
            theo.second = isotopePeaks[i].second;
            approxDist.push_back(theo);
        }
    }

    /**
     * Compute the exact theoretical fragment isotopic distribution based on the precursor isotope distribution calculator.
     * @param theoDist a vector to be filled with the theoretical isotopic distribution. Composed of a vector of pairs
     * <double, double> the first being the mz of each isotope, the second the probability of seeing the peak (equivalent
     * to the peak abundance within the distribution). Vector will be cleared before being filled with distribution.
     * @param searchDepth how many isotope peaks to report in the distribution. Search depth must be greater than 0. A
     * search depth of 1 reports only the monoisotopic peak. A search depth of 2 reports m0 and m1 peaks. ect.
     * @param ion the Ion from which the monoisotopic peak will be based.
     */
    static void exactPrecursorIsotopeDist(std::vector<std::pair<double, double> > &theoDist,
                                          const std::set<OpenMS::UInt> &precursorIsotopes, const Ion &ion)
    {
        OpenMS::UInt minIsotope = 7;

        OpenMS::UInt maxIsotope = *std::max_element(precursorIsotopes.begin(), precursorIsotopes.end());
        //construct distribution of depth at the maximum precursor isotope isolated
        OpenMS::UInt searchDepth = 7;//std::min(minIsotope, *std::max_element(precursorIsotopes.begin(), precursorIsotopes.end()) + 1);

        //clear vector for distribution
        theoDist.clear();

        //compute isotopic distribution and get vector of isotope peaks
        std::vector<std::pair<OpenMS::Size, double> > theoPeakList =
                ion.formula.getIsotopeDistribution(searchDepth).getContainer();

        //ion mz
        double ionMZ = ion.monoWeight / ion.charge;
        double basePeak = 0.0;
        for (int i = 0; i < theoPeakList.size(); ++i) basePeak = std::max(basePeak, theoPeakList[i].second);

        //loop through calculated isotopic distribution, fill with actual mz values
        for (int i = 0; i < theoPeakList.size(); ++i) {

            if (theoPeakList[i].second < basePeak * 0.1 && i > maxIsotope) continue;

            //compute mz of isotope peak
            double isoMZ = ionMZ + ( OpenMS::Constants::C13C12_MASSDIFF_U / ion.charge ) * i;

            //set theoretical distribution pair
            std::pair<double, double> theo;
            theo.first = isoMZ;
            theo.second = theoPeakList[i].second;
            theoDist.push_back(theo);
        }

        normalizeDistribution(theoDist);
    }

    /**
     * Check if a scaled isotope distribution is following a characteristic isotope distribution.
     * @param dist scaled distribution where peak intensities sum to 1
     * @return true if distribution follows a characterist rise/fall
     */
    static bool scaledDistributionValid(const std::vector<std::pair<double, double> > &dist)
    {
        return true;
        //distribution values decreasing flag
        bool valuesDecreasing = false;

        //loop from beginning of distribution, checking for decreasing values
        for (int i = 0; i < dist.size() - 1; ++i) {
            //if values are increasing
            if (!valuesDecreasing) {
                //check for next peak decreasing and difference is greater than 5%
                if ((dist[i+1].second < dist[i].second) && ((dist[i].second - dist[i+1].second) > 0.05) ) {
                    //next peak is greater than 5% less than current peak, distribution is decreasing
                    valuesDecreasing = true;
                }
            } else {    //values are decreasing
                //check for next peak increasing and difference is greater than 5%
                if ((dist[i+1].second > dist[i].second) && ((dist[i+1].second - dist[i].second) > 0.05) ) {
                    //distribution falling but next peak increases by more than 5%
                    return false;
                }
            }
        }
        //distribution follows normal rise and fall.
        return true;
    }
}


#endif //FRAGMENT_ISOTOPE_DISTRIBUTION_PAPER_SPECTRUMUTILITIES_H
