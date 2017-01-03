//
// Created by Dennis Goldfarb on 1/3/17.
//

#include <iostream>
#include <fstream>
#include <random>

#include <OpenMS/CHEMISTRY/Element.h>
#include <OpenMS/CHEMISTRY/AASequence.h>
#include <OpenMS/CHEMISTRY/ResidueDB.h>
#include <OpenMS/CHEMISTRY/ElementDB.h>

static const OpenMS::ResidueDB* residueDB = OpenMS::ResidueDB::getInstance();

static std::string AMINO_ACIDS = "ADEFGHIKLNPQRSTVWY";
static std::string AMINO_ACIDS_SULFUR = "CM";

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis_AA(0, AMINO_ACIDS.length()-1);
std::uniform_int_distribution<> dis_S(0, AMINO_ACIDS_SULFUR.length()-1);

int max_depth;

void write_distribution(OpenMS::AASequence &p, std::ofstream* outfiles) {

    OpenMS::EmpiricalFormula precursor_ef = p.getFormula();
    OpenMS::IsotopeDistribution precursor_id = precursor_ef.getIsotopeDistribution(30);

    for (int precursor_isotope = 0; precursor_isotope < max_depth; ++precursor_isotope)
    {
        outfiles[precursor_isotope] << precursor_id.getContainer()[precursor_isotope].second
                                    << "\t" << precursor_ef.getMonoWeight() << std::endl;
    }
}

OpenMS::AASequence create_random_peptide_sequence(int peptide_length, int num_sulfurs) {
    OpenMS::AASequence random_peptide;

    // for insertion of sulfur containing amino acids
    for (int i = 0; i < num_sulfurs; ++i) {
        random_peptide += residueDB->getResidue(AMINO_ACIDS_SULFUR[dis_S(gen)]);
    }

    // random amino acid insertion (non Sulfur and Selenium amino acids)
    for (int aa_index = 0; aa_index < peptide_length; ++aa_index) {
        random_peptide += residueDB->getResidue(AMINO_ACIDS[dis_AA(gen)]);
    }

    return random_peptide;
}

void sample_isotopic_distributions(std::string base_path, float max_mass, int num_samples, int num_sulfurs, bool append) {

    // create all output files and write header to each
    std::ofstream* outfiles = new std::ofstream[max_depth];
    for (int precursor_isotope = 0; precursor_isotope < max_depth; ++precursor_isotope) {
        std::string filename = "Precursor" + std::to_string(precursor_isotope) + ".tab";

        if (append)
        {
            outfiles[precursor_isotope].open(base_path + filename, std::ofstream::out | std::ofstream::app);
        }
        else
        {
            outfiles[precursor_isotope].open(base_path + filename);
        }

        // Only add the header if we're creating a new file
        // This happens if we're not appending, or if we're appending and it's our first time in the loop
        if (!append) {
            outfiles[precursor_isotope] << "probability" << "\tprecursor.mass" << std::endl;
        }
    }

    int max_length = max_mass/100;

    for (int peptide_length = 0; peptide_length <= max_length; ++peptide_length) {

        for (int sample = 0; sample < num_samples; ++sample) {

            OpenMS::AASequence random_sequence = create_random_peptide_sequence(peptide_length, num_sulfurs);

            if (random_sequence.size() > 0 && random_sequence.getMonoWeight() <= max_mass) {
                write_distribution(random_sequence, outfiles);
            }
        }
    }

    // close all output files
    for (int precursor_isotope = 0; precursor_isotope < max_depth; ++precursor_isotope) {
        outfiles[precursor_isotope].close();
    }
    delete[] outfiles;
}

void sample_average_isotopic_distribution(std::string distribution_path, std::string base_path, float max_mass, float min_percentage)
{
    std::map<int, int> sulfurs2count;

    std::ifstream sulfur_dist_in(distribution_path);
    std::string input;
    int S, CS, count, i=0;
    while (sulfur_dist_in >> input)
    {
        if (i == 0) S = atoi(input.c_str());
        else {
            count = atoi(input.c_str());
            sulfurs2count[S] = count;
        }
        i = (i+1) % 2;
    }

    int max_count = 0;
    for (auto itr : sulfurs2count)
    {
        max_count = std::max(max_count, itr.second);
    }

    bool append = false;
    for (auto itr : sulfurs2count)
    {
        double percentage = (double) itr.second / max_count;
        if (percentage >= min_percentage) {
            int num_samples = std::floor(percentage / min_percentage);
            sample_isotopic_distributions(base_path, max_mass, num_samples, itr.first, append);
            append = true;
        }
    }
}


void usage()
{
    std::cout << "GenerateTrainingData 0 out_path max_mass num_samples S max_isotope" << std::endl;
    std::cout << "out_path: The path to the directory that will store the training data, e.g. ~/data/" << std::endl;
    std::cout << "max_mass: maximum mass allowed for sampled peptides, e.g. 8500" << std::endl;
    std::cout << "num_samples: number of random peptides to generate for each peptide length, e.g 100" << std::endl;
    std::cout << "S: number of sulfurs that should be in the fragment ion, e.g. 0" << std::endl;
    std::cout << "max_isotope: The maximum isotope to generate training data for, e.g. 5" << std::endl;
    std::cout << std::endl;

    std::cout << "GenerateTrainingData 1 sulfur_corrected sulfur_dist_path out_path max_mass min_percentage max_isotope" << std::endl;
    std::cout << "sulfur_dist_path: file path to the results of GetSulfurDistribution, e.g. ~/data/sulfur_distribution.tab" << std::endl;
    std::cout << "out_path: The path to the directory that will store the training data, e.g. ~/data/" << std::endl;
    std::cout << "max_mass: maximum mass allowed for sampled peptides, e.g. 8500" << std::endl;
    std::cout << "min_percentage: the min abundance of a sulfur distribution necessary to be included in the training data (relative to most abundant case), e.g. .001" << std::endl;
    std::cout << "max_isotope: The maximum isotope to generate training data for, e.g. 5" << std::endl;
}


int main(int argc, const char ** argv) {
    if (argc != 7)
    {
        usage();
    }

    int mode = atoi(argv[1]);

    if (mode == 0)
    {
        std::string out_path = argv[2];
        float max_mass = atof(argv[3]);
        int num_samples = atoi(argv[4]);
        int S = atoi(argv[5]);

        max_depth = atoi(argv[6]) + 1;

        sample_isotopic_distributions(out_path, max_mass, num_samples, S, false);
    }
    else if (mode == 1)
    {
        std::string dist_path = argv[2];
        std::string out_path = argv[3];
        float max_mass = atof(argv[4]);
        float min_percentage = atof(argv[5]);

        max_depth = atoi(argv[6]) + 1;

        sample_average_isotopic_distribution(dist_path, out_path, max_mass, min_percentage);
    }

    return 0;
}