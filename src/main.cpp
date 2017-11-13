#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "chain.hpp"

#include "spoa/spoa.hpp"

static struct option options[] = {
    {"match", required_argument, 0, 'm'},
    {"mismatch", required_argument, 0, 'x'},
    {"gap-open", required_argument, 0, 'o'},
    {"gap-extend", required_argument, 0, 'e'},
    {"algorithm", required_argument, 0, 'l'},
    {"result", required_argument, 0, 'r'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

void help();

int main(int argc, char** argv) {

    int8_t match = 5;
    int8_t mismatch = -4;
    int8_t gap = -8;

    uint8_t algorithm = 0;
    uint8_t result = 0;

    char argument;
    while ((argument = getopt_long(argc, argv, "m:x:g:l:r:h", options, nullptr)) != -1) {
        switch (argument) {
            case 'm':
                match = atoi(optarg);
                break;
            case 'x':
                mismatch = atoi(optarg);
                break;
            case 'g':
                gap = atoi(optarg);
                break;
            case 'l':
                algorithm = atoi(optarg);
                break;
            case 'r':
                result = atoi(optarg);
                break;
            case 'h':
            default:
                help();
                return -1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "spoa:: error: missing input file!\n");
        help();
        return -1;
    }

    std::string input_path = argv[optind];
    auto extension = input_path.substr(input_path.rfind('.'));

    if (extension != ".fasta" && extension != ".fa" && extension != ".fastq" &&
        extension != ".fq") {
        fprintf(stderr, "spoa:: error: "
            "file %s has unsupported format extension (valid extensions: "
            ".fasta, .fa, .fastq, .fq)!\n", input_path.c_str());
        return -1;
    }

    auto alignment_engine = spoa::createAlignmentEngine(
        static_cast<spoa::AlignmentType>(algorithm), match, mismatch, gap);

    auto graph = spoa::createGraph();

    std::vector<std::unique_ptr<spoa::Chain>> chains;

    if (extension == ".fasta" || extension == ".fa") {
        auto creader = bioparser::createReader<spoa::Chain,
            bioparser::FastaReader>(input_path);
        creader->read_objects(chains, -1);

        size_t max_sequence_size = 0;
        for (const auto& it: chains) {
            max_sequence_size = std::max(max_sequence_size, it->data().size());
        }
        alignment_engine->prealloc(max_sequence_size, 4);

        for (const auto& it: chains) {
            auto alignment = alignment_engine->align_sequence_with_graph(
                it->data(), graph);
            graph->add_alignment(alignment, it->data());
        }
    } else if (extension == ".fastq" || extension == ".fq") {
        auto creader = bioparser::createReader<spoa::Chain,
            bioparser::FastqReader>(input_path);
        creader->read_objects(chains, -1);

        size_t max_sequence_size = 0;
        for (const auto& it: chains) {
            max_sequence_size = std::max(max_sequence_size, it->data().size());
        }
        alignment_engine->prealloc(max_sequence_size, 4);

        for (const auto& it: chains) {
            auto alignment = alignment_engine->align_sequence_with_graph(
                it->data(), graph);
            graph->add_alignment(alignment, it->data(), it->quality());
        }
    }

    if (result == 0 || result == 2) {
        std::string consensus = graph->generate_consensus();
        fprintf(stdout, "Consensus (%zu)\n", consensus.size());
        fprintf(stdout, "%s\n", consensus.c_str());
    }

    if (result == 1 || result == 2) {
        std::vector<std::string> msa;
        graph->generate_multiple_sequence_alignment(msa);
        fprintf(stdout, "Multiple sequence alignment\n");
        for (const auto& it: msa) {
            fprintf(stdout, "%s\n", it.c_str());
        }
    }

    return 0;
}

void help() {
    printf(
        "usage: spoa [options ...] <sequences>\n"
        "\n"
        "    <sequences>\n"
        "        input file in FASTA/FASTQ format containing sequences\n"
        "\n"
        "    options:\n"
        "        -m, --match <int>\n"
        "            default: 5\n"
        "            score for matching bases\n"
        "        -x, --mismatch <int>\n"
        "            default: -4\n"
        "            score for mismatching bases\n"
        "        -g, --gap <int>\n"
        "            default: -8\n"
        "            gap penalty (must be negative)\n"
        "        -l, --algorithm <int>\n"
        "            default: 0\n"
        "            alignment mode:\n"
        "                0 - local (Smith-Waterman)\n"
        "                1 - global (Needleman-Wunsch)\n"
        "                2 - semi-global\n"
        "        -r, --result <int>\n"
        "            default: 0\n"
        "            result mode:\n"
        "                0 - consensus\n"
        "                1 - multiple sequence alignment\n"
        "                2 - 0 + 1\n"
        "        -h, --help\n"
        "            prints out the help\n");
}
