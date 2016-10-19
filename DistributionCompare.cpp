//

#include <OpenMS/FORMAT/MzMLFile.h>
#include <OpenMS/FORMAT/IdXMLFile.h>
#include <OpenMS/ANALYSIS/ID/IDMapper.h>
#include <OpenMS/CHEMISTRY/TheoreticalSpectrumGenerator.h>
#include <OpenMS/CHEMISTRY/ElementDB.h>
#include "Ion.h"
#include "Stats.h"
#include <OpenMS/CHEMISTRY/IsotopeDistribution.h>
#include <OpenMS/MATH/STATISTICS/StatisticFunctions.h>
#include <fstream>

//global variables
const double FDR_THRESHOLD = 0.01;          //False discovery rate threshold (%/100)
const double ERROR_PPM = 20 * 0.000001;     //acquisition mz error for peak matching
const double NEUTRON_MASS = 1.008701;       //mass of a neutron
const double ISOLATION_WINDOW_MZ = 1.6;     //the isolation window used for precursor ion collection
const OpenMS::ElementDB* ELEMENTS = OpenMS::ElementDB::getInstance();

/**
 * Function to determine which precursor isotopes were captured within the ms2 isolation window
 * @param precursorIsotopes a vector of ints to be filled with the representation of which precursor isotopes were
 * captured in the isolation window. A vector <0, 1, 2> would represent the m0, m1, and m2 isotopes of an isotopic
 * distribution. Vector will be cleared before being filled with int values.
 * @param precursorMonoWeight mono isotopic weight of the precursor peptide
 * @param precursorCharge the charge of the precursor peptide ion identified from current MS2 spectrum
 * @param ms2mz the mz center of the isolation window used for current MS2 spectrum
 */
void whichPrecursorIsotopes(std::vector<OpenMS::UInt> &precursorIsotopes, const double precursorMonoWeight,
                            const OpenMS::Int &precursorCharge, const double ms2mz)
{
    //mz of precursor peptide
    double precursorMZ = precursorMonoWeight / precursorCharge;

    //distance between isotopic peaks based on precursor charge
    double isotopicStep = NEUTRON_MASS / precursorCharge;

    //ms2 isolation window centered on which precursor isotope
    int centeredPrecursorIsotope = int(std::round((ms2mz - precursorMZ) / isotopicStep));

    //number of isotopes to either side of centered precursor isotope that are captured in isolation window
    int isotopesCapturedPerSide = int( std::floor( (ISOLATION_WINDOW_MZ / 2) / isotopicStep ) );

    //starting isotope of the distribution captured in the isolation window
    int startingIsotope;
    if (isotopesCapturedPerSide > centeredPrecursorIsotope) {
        startingIsotope = 0;
    } else {
        startingIsotope = centeredPrecursorIsotope - isotopesCapturedPerSide;
    }

    //ending isotope of the distribution captured in the isolation window
    int endingIsotope = centeredPrecursorIsotope + isotopesCapturedPerSide;

    //clear precursor isotopes vector
    precursorIsotopes.clear();
    //fill precursor isotopes vector
    for (int i = startingIsotope; i <= endingIsotope; ++i) {
        OpenMS::UInt isoPeak = i;
        precursorIsotopes.push_back(isoPeak);
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
void exactPrecursorIsotopeDist(std::vector<std::pair<double, double> > &theoDist,
                               const int searchDepth, const Ion &ion)
{
    //search depth 0 reports all possible!
    //based on previous checks, this shouldn't be tested
    if (searchDepth == 0) {
        return;
    }

    //clear vector for distribution
    theoDist.clear();

    //compute isotopic distribution and get vector of isotope peaks
    std::vector<std::pair<OpenMS::Size, double> > theoPeakList =
            ion.formula.getIsotopeDistribution(searchDepth).getContainer();

    //ion mz
    double ionMZ = ion.monoWeight / ion.charge;

    //loop through calculated isotopic distribution, fill with actual mz values
    for (int i = 0; i < theoPeakList.size(); ++i) {

        //compute mz of isotope peak
        double isoMZ = ionMZ + ( NEUTRON_MASS / ion.charge ) * i;

        //set theoretical distribution pair
        std::pair<double, double> theo;
        theo.first = isoMZ;
        theo.second = theoPeakList[i].second;
        theoDist.push_back(theo);
    }
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
void exactConditionalFragmentIsotopeDist(std::vector<std::pair<double, double> > &condDist,
                                         const std::vector<OpenMS::UInt> &precursorIsotopes,
                                         const Ion &ion,
                                         const OpenMS::AASequence &precursorSequence,
                                         const OpenMS::Int &precursorCharge)
{
    //clear vector for distribution
    condDist.clear();

    //compute conditional isotopic distribution and get vector of isotope peaks
    std::vector<std::pair<OpenMS::Size, double> > condPeakList =
            ion.formula.getConditionalFragmentIsotopeDist(
                    precursorSequence.getFormula(OpenMS::Residue::Full, precursorCharge),precursorIsotopes).getContainer();

    //ion mz
    double ionMZ = ion.monoWeight / ion.charge;

    //loop through calculated isotopic distribution, fill with actual mz values
    for (int i = 0; i < condPeakList.size(); ++i) {

        //compute mz of isotope peak
        double isoMZ = ionMZ + ( NEUTRON_MASS / ion.charge ) * i;

        //set theoretical distribution pair
        std::pair<double, double> theo;
        theo.first = isoMZ;
        theo.second = condPeakList[i].second;
        condDist.push_back(theo);
    }
}

void approxPrecursorFromWeightIsotopeDist(std::vector<std::pair<double, double> > &approxDist,
                                          const std::vector<OpenMS::UInt> &precursorIsotopes,
                                          const Ion &fragmentIon)
{
    //clear vector for distribution
    approxDist.clear();

    //construct distribution of depth at the maximum precursor isotope isolated
    OpenMS::IsotopeDistribution fragmentDist(precursorIsotopes.back() + 1);

    //estimate from fragment average weight
    fragmentDist.estimateFromPeptideWeight(fragmentIon.formula.getAverageWeight());

    //get isotope vector
    std::vector<std::pair<OpenMS::Size, double> > isotopePeaks = fragmentDist.getContainer();

    //ion mz
    double ionMZ = fragmentIon.monoWeight / fragmentIon.charge;

    //loop through calculated isotopic distribution, fill with actual mz values
    for (int i = 0; i < isotopePeaks.size(); ++i) {

        //compute mz of isotope peak
        double isoMZ = ionMZ + ( NEUTRON_MASS / fragmentIon.charge ) * i;

        //set distribution pair
        std::pair<double, double> theo;
        theo.first = isoMZ;
        theo.second = isotopePeaks[i].second;
        approxDist.push_back(theo);
    }
}

void approxFragmentFromWeightIsotopeDist(std::vector<std::pair<double, double> > &approxDist,
                                         const std::vector<OpenMS::UInt> &precursorIsotopes,
                                         const Ion &fragmentIon,
                                         const OpenMS::AASequence &precursorSequence,
                                         const OpenMS::Int &precursorCharge)
{
    //clear vector for distribution
    approxDist.clear();

    //precursor average weight
    double precursorAvgWeight = precursorSequence.getAverageWeight(OpenMS::Residue::Full, precursorCharge);
    //fragment average weight
    double fragmentAvgWeight = fragmentIon.sequence.getAverageWeight(OpenMS::Residue::Full, fragmentIon.charge);

    //construct distribution
    OpenMS::IsotopeDistribution fragmentDist(precursorIsotopes.back() + 1);

    //estimate approx distribution from peptide weight
    fragmentDist.estimateForFragmentFromPeptideWeight(precursorAvgWeight, fragmentAvgWeight, precursorIsotopes);

    //get isotope vector
    std::vector<std::pair<OpenMS::Size, double> > isotopePeaks = fragmentDist.getContainer();

    //ion mz
    double ionMZ = fragmentIon.monoWeight / fragmentIon.charge;

    //loop through calculated isotopic distribution, fill with actual mz values
    for (int i = 0; i < isotopePeaks.size(); ++i) {

        //compute mz of isotope peak
        double isoMZ = ionMZ + ( NEUTRON_MASS / fragmentIon.charge ) * i;

        //set distribution pair
        std::pair<double, double> theo;
        theo.first = isoMZ;
        theo.second = isotopePeaks[i].second;
        approxDist.push_back(theo);
    }
}

void approxFragmentFromWeightAndSIsotopeDist(std::vector<std::pair<double, double> > &approxDist,
                                             const std::vector<OpenMS::UInt> &precursorIsotopes,
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
    double fragmentAvgWeight = fragmentIon.sequence.getAverageWeight(OpenMS::Residue::Full, fragmentIon.charge);
    //fragment number of sulfurs
    int fragmentSulfurs = fragmentIon.formula.getNumberOf(ELEMENTS->getElement("Sulfur"));

    //construct distribution
    OpenMS::IsotopeDistribution fragmentDist(precursorIsotopes.back() + 1);

    //estimate approx distribution from peptide weight
    fragmentDist.estimateForFragmentFromPeptideWeightAndS(precursorAvgWeight, precursorSulfurs,
                                                          fragmentAvgWeight, fragmentSulfurs,
                                                          precursorIsotopes);

    //get isotope vector
    std::vector<std::pair<OpenMS::Size, double> > isotopePeaks = fragmentDist.getContainer();

    //ion mz
    double ionMZ = fragmentIon.monoWeight / fragmentIon.charge;

    //loop through calculated isotopic distribution, fill with actual mz values
    for (int i = 0; i < isotopePeaks.size(); ++i) {

        //compute mz of isotope peak
        double isoMZ = ionMZ + ( NEUTRON_MASS / fragmentIon.charge ) * i;

        //set distribution pair
        std::pair<double, double> theo;
        theo.first = isoMZ;
        theo.second = isotopePeaks[i].second;
        approxDist.push_back(theo);
    }
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
void observedDistribution(std::vector<std::pair<double, double> > &obsDist,
                          std::vector<std::pair<double, double> > &theoDist,
                          const OpenMS::MSSpectrum<OpenMS::Peak1D> &spec)
{
    //loop through each theoretical peak in isotopic distribution
    for (int i = 0; i < theoDist.size(); ++i) {

        //calculate search tolerance
        double tol = ERROR_PPM * theoDist[i].first;

        //find index of actual peak in spectrum
        OpenMS::Int isoPeakIndex = spec.findNearest(theoDist[i].first, tol);

        //observed isotope distribution pair
        std::pair<double, double> obs;

        if (isoPeakIndex == -1) {
            //peak not found
            obs.first = theoDist[i].first;
            obs.second = 0;
        } else {
            //peak found
            obs.first = spec[isoPeakIndex].getMZ();
            obs.second = spec[isoPeakIndex].getIntensity();
        }
        //add observed peak pair to vector of observed isotope distribution
        obsDist.push_back(obs);
    }
}

/**
 * Scales an isotopic distribution of peaks based on raw intensity to relative intensity which sum to 1 accross
 * all peaks in the distribution.
 * @param obsDist a vector of observed peaks within an isotopic distribution. Composed of a vector of pairs
 * <double, double> the first being the mz of each isotope, the second being the raw intensity of the peak.
 * Vector will be modified to contain scaled intensity values instead of raw intensity values.
 */
void scaleDistribution(std::vector<std::pair<double, double> > &obsDist)
{
    //sum intensities across distribution
    double totalIntensity = 0;
    for (int i = 0; i < obsDist.size(); ++i) {
        totalIntensity += obsDist[i].second;
    }

    //compute scaled intensity and replace value
    for (int j = 0; j < obsDist.size(); ++j) {
        obsDist[j].second = obsDist[j].second / totalIntensity;
    }
}

/**
 * Check an isotopic distribution of greater than 3 peaks to see if peaks follow a typical descending distribution.
 * @param dist a vector of observed peaks within an isotopic distribution. Composed of a vector of pairs
 * <double, double> the first being the mz of each isotope, the second being the intensity or abundance of the peak.
 * @return true if the distribution peaks decrease in intensity or abundance past the m1 peak. False otherwise.
 */
bool distributionValid(std::vector<std::pair<double, double> > &dist)
{
    //only check if distribution is at least 3 deep
    if (dist.size() < 3) {
        return true;
    }

    //loop from end of distribution up to the m1 peak
    for (int i = dist.size() - 1; i > 1; --i) {
        //if intensity at m_i is greater than intensity at m_i-1
        if (dist[i].second > dist[i-1].second) {
            return false;
        }
    }

    return true;
}

double computeCC(const std::vector<std::pair<double, double> > &obsDist,
                 const std::vector<std::pair<double, double> > &theoDist)
{
    //vector to hold observed proportions
    std::vector<double> obsProp;
    //vector to hold theoretical proportions
    std::vector<double> theoProp;

    //check they are both the same size
    if (obsDist.size() != theoDist.size()) {
        return 0;
    }

    //fill proportions vectors from distribution parameters
    for (int i = 0; i < obsDist.size(); ++i) {
        obsProp.push_back(obsDist[i].second);
        theoProp.push_back(theoDist[i].second);
    }

    //compute pearsons correlation coefficient
    return OpenMS::Math::pearsonCorrelationCoefficient(obsProp.begin(), obsProp.end(),
                                                       theoProp.begin(), theoProp.end());
}

double computeX2(const std::vector<std::pair<double, double> > &obsDist,
                 const std::vector<std::pair<double, double> > &theoDist)
{
    //vector to hold observed proportions
    std::vector<double> obsProp;
    //vector to hold theoretical proportions
    std::vector<double> theoProp;

    //check they are both the same size
    if (obsDist.size() != theoDist.size()) {
        return -1;
    }

    //fill proportions vectors from distribution parameters
    for (int i = 0; i < obsDist.size(); ++i) {
        obsProp.push_back(obsDist[i].second);
        theoProp.push_back(theoDist[i].second);
    }

    //compute chi squared statistic
    return Stats::chiSquared(obsProp.begin(), obsProp.end(),
                             theoProp.begin(), theoProp.end());
}

double computeVD(const std::vector<std::pair<double, double> > &obsDist,
                 const std::vector<std::pair<double, double> > &theoDist)
{
    //vector to hold observed proportions
    std::vector<double> obsProp;
    //vector to hold theoretical proportions
    std::vector<double> theoProp;

    //check they are both the same size
    if (obsDist.size() != theoDist.size()) {
        return -1;
    }

    //fill proportions vectors from distribution parameters
    for (int i = 0; i < obsDist.size(); ++i) {
        obsProp.push_back(obsDist[i].second);
        theoProp.push_back(theoDist[i].second);
    }

    //compute total variation distance
    return Stats::totalVariationDistance(obsProp.begin(), obsProp.end(),
                                         theoProp.begin(), theoProp.end());
}

void usage(){
    std::cout << "usage: SpecOps data_directory input_mzML_spectra_file input_idXML_PSM_file" << std::endl;
    std::cout << "  data_directory: full path to directory containing input data";
    std::cout << " and destination for output files" << std::endl;
    std::cout << "  input_mzML_spectra_file: input .mzML file contained in the data directory" << std::endl;
    std::cout << "  input_idXML_PSM_file: input .idXML file contained in the data directory" << std::endl;
}

int main(int argc, char * argv[])
{
    //check for correct number of command line arguments
    if (argc != 4) {
        usage();
        return 0;
    }
    //report data directory
    const std::string dataDir = argv[1];
    std::cout << "Data directory: " << dataDir << std::endl;
    //report mzML file
    const std::string mzMLFileName = argv[2];
    std::cout << "Input mzML spectra file: " << mzMLFileName << std::endl;
    //report idXML file
    const std::string idXMLFileName = argv[3];
    std::cout << "Input idXML PSMs file: " << idXMLFileName << std::endl;

    //Load input mzML file into MSExperiment
    OpenMS::MzMLFile mzMLDataFile;
    OpenMS::MSExperiment<OpenMS::Peak1D> msExperiment;
    std::cout << "Loading input mzML file " << mzMLFileName << "..." << std::endl;
    try {
        mzMLDataFile.load(dataDir + mzMLFileName, msExperiment);
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        usage();
        return 0;
    }

    //load input idXML file
    OpenMS::IdXMLFile peptideDataFile;
    std::vector<OpenMS::ProteinIdentification> protIDs;
    std::vector<OpenMS::PeptideIdentification> pepIDs;
    std::cout << "Loading input idXML file " << idXMLFileName << "..." << std::endl;
    try {
        peptideDataFile.load(dataDir + idXMLFileName, protIDs, pepIDs);
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        usage();
        return 0;
    }

    //Report spectra loaded and PSMs loaded
    std::cout << "Number of spectra loaded: " << msExperiment.getNrSpectra() << std::endl;
    std::cout << "Number of peptide identifications (PSMs): " << pepIDs.size() << std::endl;

    //Map peptide identifications with spectra
    std::cout << "Mapping PSMs to associated spectra..." << std::endl;
    OpenMS::IDMapper mapper;
    try {
        mapper.annotate(msExperiment, pepIDs, protIDs);
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        return 0;
    }
    //Mapping statistics reported by .annotate to std out

    //reporting variables
    int ionID = 0;
    int numMatchedIons = 0;
    int numSearchedAtDepth[10] = {0};
    int numMatchedAtDepth[10] = {0};
    int numCompleteDists[10] = {0};
    int numPrecursAtCharge[10] = {0};
    int numPeptideHits = 0;
    int numPeptideHitsBelowFDR = 0;

    //output file for distribution comparison results
    const std::string scoreFileName = "distributionScores.out";
    std::ofstream distributionScoreFile;
    try {
        distributionScoreFile.open(dataDir + scoreFileName);
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        usage();
        return 0;
    }
    //headers for distribution score output file
    distributionScoreFile << "ionID\t";                       //ion ID of monoisotopic ion
    distributionScoreFile << "distributionValid\t";           //check for valid distribution
    distributionScoreFile << "distributionMonoWeight\t";      //ion distribution monoisotopic weight
    distributionScoreFile << "ionCharge\t";                   //ion distribtuion charge
    distributionScoreFile << "searchDepth\t";                 //distribution search depth
    distributionScoreFile << "openMSPearsonCC\t";             //pearsonCC for OpenMS distribution
    distributionScoreFile << "conditionalPearsonCC\t";        //pearsonCC for Dennis's conditional distribution
    distributionScoreFile << "openMSChiSquared\t";            //chi-squared for OpenMS distribution
    distributionScoreFile << "conditionalChiSquared\t";       //chi-squared for Dennis's cond. distribution
    distributionScoreFile << "openMSTotVarDist\t";            //total variation distance for OpenMS distribution
    distributionScoreFile << "conditionalTotVarDist\t";       //total variation distance for cond. distribution
    distributionScoreFile << "completeFlag\t";                //full ion distribution identified in spectra
    distributionScoreFile << "completeAtDepth\n";             //ion distribution complete up to depth


    //output file for ion identification data
    const std::string ionFileName = "ions.out";
    std::ofstream ionFile;
    try {
        ionFile.open(dataDir + ionFileName);
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        usage();
        return 0;
    }
    //headers for ion identification file
    ionFile << "ionID\t";               //unique ion ID
    ionFile << "spectrumIndex\t";       //spectrum index
    ionFile << "PSMindex\t";            //PSM index
    ionFile << "peptideHitIndex\t";     //peptide hit index
    ionFile << "precursorSequence\t";   //precursor peptide sequence
    ionFile << "precursorCharge\t";     //precursor peptide charge
    ionFile << "ionSequence\t";         //ion sequence
    ionFile << "ionType\t";             //ion type
    ionFile << "ionCharge\t";           //ion charge
    ionFile << "ionMolFormula\t";       //ion formula
    ionFile << "ionMonoWeight\t";       //ion weight
    ionFile << "ionMZ\t";               //ion mz
    ionFile << "ionSearchTolerance\t";  //ion search tolerance
    ionFile << "ionFoundFlag\n";        //ion not found


    //Loop through all spectra
    for (int specIndex = 0; specIndex < msExperiment.getNrSpectra(); ++specIndex) {
        //sort spectrum by mz
        msExperiment.getSpectrum(specIndex).sortByPosition();

        //get copy of current spectrum
        const OpenMS::MSSpectrum<OpenMS::Peak1D> spec = msExperiment.getSpectrum(specIndex);

        //get peptide identifications
        const std::vector<OpenMS::PeptideIdentification> pepIDs = spec.getPeptideIdentifications();

        //Loop through each peptide identification (PSM)
        for (int pepIDIndex = 0; pepIDIndex < pepIDs.size(); ++pepIDIndex) {
            //get peptide hits
            const std::vector<OpenMS::PeptideHit> pepHits = pepIDs[pepIDIndex].getHits();

            //get ms2 mz
            double ms2mz = pepIDs[pepIDIndex].getMZ();

            //loop through each peptide hit
            for (int pepHitIndex = 0; pepHitIndex < pepHits.size(); ++pepHitIndex) {
                ++numPeptideHits;

                //if peptide score is above FDR threshold, skip to next peptide
                if (pepHits[pepHitIndex].getScore() >= FDR_THRESHOLD) {
                    continue;
                }

                ++numPeptideHitsBelowFDR;

                //get AASequence and charge from peptide hit
                const OpenMS::AASequence pepSeq = pepHits[pepHitIndex].getSequence();
                const OpenMS::Int pepCharge = pepHits[pepHitIndex].getCharge();

                //record number of precursors at each charge state
                ++numPrecursAtCharge[pepCharge];

                //if charge state 1, skip to next peptide hit
                if (pepCharge == 1) {
                    continue;
                }

                //create list of b and y ions
                std::vector<Ion> ionList;
                Ion::generateFragmentIons(ionList, pepSeq, pepCharge);

                //create array of found not found for corresponding ion list
                //std::vector<bool> ionFound;

                //loop through each ion
                for (int ionIndex = 0; ionIndex < ionList.size(); ++ionIndex) {
                    //update ionID
                    ++ionID;

                    //ion mz
                    double mz = ionList[ionIndex].monoWeight / ionList[ionIndex].charge;

                    //compute search peak matching tolerance
                    double tol = ERROR_PPM * mz;

                    //find nearest peak to ion mz within tolerance
                    OpenMS::Int peakIndex = msExperiment.getSpectrum(specIndex).findNearest(mz, tol);

                    //write ion information to file
                    ionFile << ionID << "\t";           //unique ion ID
                    ionFile << specIndex << "\t";       //spectrum index
                    ionFile << pepIDIndex << "\t";      //PSM index
                    ionFile << pepHitIndex << "\t";     //peptide hit index
                    ionFile << pepSeq << "\t";          //precursor peptide sequence
                    ionFile << pepCharge << "\t";       //precursor peptide charge
                    ionFile << ionList[ionIndex].sequence << "\t";      //ion sequence
                    ionFile << ionList[ionIndex].type << "\t";          //ion type
                    ionFile << ionList[ionIndex].charge << "\t";        //ion charge
                    ionFile << ionList[ionIndex].formula << "\t";       //ion formula
                    ionFile << ionList[ionIndex].monoWeight << "\t";    //ion weight
                    ionFile << mz << "\t";              //ion mz
                    ionFile << tol << "\t";             //ion search tolerance


                    if (peakIndex == -1) {
                        //ion not found
                        //write ion information to file
                        ionFile << false << "\n";       //ion not found
                    } else {
                        //ion found
                        ++numMatchedIons;
                        //write ion information to file
                        ionFile << true << "\n";        //ion found

                        //vector for exact theoretical precursor isotope distribution <mz, probability>
                        std::vector<std::pair<double, double> > theoDist;
                        //vector for exact conditional fragment isotope distribution <mz, probability>
                        std::vector<std::pair<double, double> > condDist;

                        //vector for approximate precursor isotope distribution from peptide weight <mz, probability>
                        std::vector<std::pair<double, double> > approxPrecursorDist;
                        //vector for approximate fragment isotope distribution from peptide weight <mz, probability>
                        std::vector<std::pair<double, double> > approxFragmentDist;
                        //vector for approximate fragment isotope distribution from peptide weight and sulfurs <mz, probability>
                        std::vector<std::pair<double, double> > approxFragmentSulfurDist;

                        //vector for observed isotope distribution <mz, intensity>
                        std::vector<std::pair<double, double> > obsDist;
                        //vector for precursor isotopes captured in isolation window
                        std::vector<OpenMS::UInt> precursorIsotopes;

                        //fill precursor isotopes vector
                        whichPrecursorIsotopes(precursorIsotopes,
                                               pepSeq.getMonoWeight(OpenMS::Residue::Full, pepCharge), pepCharge, ms2mz);

                        //fill exact theoretical precurosr isotope distribution vector
                        exactPrecursorIsotopeDist(theoDist, precursorIsotopes.size(), ionList[ionIndex]);

                        //fill exact conditional isotope distribution vector
                        exactConditionalFragmentIsotopeDist(condDist, precursorIsotopes, ionList[ionIndex], pepSeq,
                                                            pepCharge);

                        //fill approx precursor isotope distribution
                        approxPrecursorFromWeightIsotopeDist(approxPrecursorDist, precursorIsotopes, ionList[ionIndex]);
                        //fill approx fragment isotope distribution
                        approxFragmentFromWeightIsotopeDist(approxFragmentDist, precursorIsotopes, ionList[ionIndex],
                                                            pepSeq, pepCharge);
                        //fill approx fragment isotope distribution with sulfurs
                        approxFragmentFromWeightAndSIsotopeDist(approxFragmentSulfurDist, precursorIsotopes,
                                                                ionList[ionIndex], pepSeq, pepCharge);

                        //match theoretical distribution with observed peaks
                        observedDistribution(obsDist, theoDist, spec);

                        //scale observed intensities across distribution
                        scaleDistribution(obsDist);

                        //compute pearsonCC with observed to OpenMS
                        double openCC = computeCC(obsDist, theoDist);

                        //compute pearsonCC with observed to Conditional
                        double condCC = computeCC(obsDist, condDist);

                        //compute chi-squared with observed to OpenMS
                        double openX2 = computeX2(obsDist, theoDist);

                        //compute chi-squared with observed to Conditional
                        double condX2 = computeX2(obsDist, condDist);

                        //compute total variation distance with observed to OpenMS
                        double openVD = computeVD(obsDist, theoDist);

                        //compute total variation distance with observed to Conditional
                        double condVD = computeVD(obsDist, condDist);

                        //report on matched ion distribution depth
                        bool completeFlag = true;
                        int completeAtDepth = 0;
                        for (int i = 0; i < obsDist.size(); ++i) {
                            if (obsDist[i].second != 0) {
                                ++numMatchedAtDepth[i];
                                if (completeFlag) {
                                    ++completeAtDepth;
                                }
                            } else {
                                completeFlag = false;
                            }
                        }
                        ++numSearchedAtDepth[theoDist.size()];
                        if (completeFlag) {
                            ++numCompleteDists[theoDist.size()];
                        }

                        //write distribution results to file
                        distributionScoreFile << ionID << "\t";             //ion ID
                        distributionScoreFile << distributionValid(obsDist) << "\t";      //valid distribution flag
                        distributionScoreFile << ionList[ionIndex].monoWeight << "\t";    //ion distribution monoisotopic weight
                        distributionScoreFile << ionList[ionIndex].charge << "\t";        //ion distribution charge
                        distributionScoreFile << theoDist.size() << "\t";                 //distribution search depth
                        distributionScoreFile << openCC << "\t";            //pearsonsCC for OpenMS distribution
                        distributionScoreFile << condCC << "\t";            //pearsonCC for Dennis's conditional distribution
                        distributionScoreFile << openX2 << "\t";            //chi-squared for OpenMS distribution
                        distributionScoreFile << condX2 << "\t";            //chi-squared for Dennis's cond. distribution
                        distributionScoreFile << openVD << "\t";            //tot. variation dist. for OpenMS dist.
                        distributionScoreFile << condVD << "\t";            //tot. variation dist. for cond. dist.
                        distributionScoreFile << completeFlag << "\t";      //complete distribution found
                        distributionScoreFile << completeAtDepth << "\n";   //complete distribution up to depth

                        /*
                        //report complete distributions
                        if (completeFlag && obsDist.size() >= 4) {

                            for (int i = 0; i < theoDist.size(); ++i) {
                                std::cout << "Theo: mz: " << theoDist[i].first << " prop: " << theoDist[i].second;
                                std::cout << " Obs: mz: " << obsDist[i].first << " prop: " << obsDist[i].second;
                                std::cout << " Cond: mz: " << condDist[i].first << " prop: " << condDist[i].second;
                                std::cout << std::endl;
                            }
                            //std::cout << "OpenMS Score: " << openMSScore << std::endl;
                            //std::cout << "Cond Score: " << condScore << std::endl;

                            //report pearsonCC
                            std::cout << "openCC= " << openCC;
                            std::cout << " condCC= " << condCC << std::endl;
                            std::cout << "openX2= " << openX2;
                            std::cout << " condX2= " << condX2 << std::endl;
                            std::cout << "***************************" << std::endl;
                        }*/
                    }
                }//ion loop
            }//peptide hit loop
        }//PSM loop
    }//spectrum loop

    for (int i = 0; i < 10; ++i) {
        std::cout << "Number of precursors at charge " << i << ": ";
        std::cout << numPrecursAtCharge[i] << std::endl;
    }
    std::cout << "Number of matched monoisotopic ions: " << numMatchedIons << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::cout << "Number of isotope spectra searched at depth " << i << ": ";
        std::cout << numSearchedAtDepth[i] << std::endl;
    }
    for (int i = 0; i < 10; ++i) {
        std::cout << "Number of ions matched at isotope " << i << ": ";
        std::cout << numMatchedAtDepth[i] << std::endl;
    }
    for (int i = 0; i < 10; ++i) {
        std::cout << "Number of complete distributions of depth " << i << ": ";
        std::cout << numCompleteDists[i] << std::endl;
    }

    //report on peptide hits
    std::cout << "Peptide hits: " << numPeptideHits << std::endl;
    std::cout << "Peptdie hits below FDR: " << numPeptideHitsBelowFDR << std::endl;
    std::cout << "Peptide hits below FDR/peptide hits: " << numPeptideHitsBelowFDR / double(numPeptideHits) << std::endl;

    //close output files
    std::cout << "Distribution comparison scorefile written to: " + scoreFileName << std::endl;
    distributionScoreFile.close();
    std::cout << "Complete ion file written to: " + ionFileName << std::endl;
    ionFile.close();

    return 0;
}
