#include <cassert>
#include <getopt.h>
#include <iostream>

#include <LogCabin/Client.h>
#include <LogCabin/Debug.h>
#include <LogCabin/Util.h>

namespace {

using LogCabin::Client::Cluster;
using LogCabin::Client::Tree;
using LogCabin::Client::Util::parseNonNegativeDuration;

/**
 * Parses argv for the main function.
 */
class OptionParser {
  public:
    OptionParser(int& argc, char**& argv)
        : argc(argc)
        , argv(argv)
        , cluster("logcabin:5254")
        , logPolicy("")
        , timeout(parseNonNegativeDuration("0s"))
    {
        while (true) {
            static struct option longOptions[] = {
               {"cluster",  required_argument, NULL, 'c'},
               {"help",  no_argument, NULL, 'h'},
               {"timeout",  required_argument, NULL, 't'},
               {"verbose",  no_argument, NULL, 'v'},
               {"verbosity",  required_argument, NULL, 256},
               {0, 0, 0, 0}
            };
            int c = getopt_long(argc, argv, "c:t:hv", longOptions, NULL);

            // Detect the end of the options.
            if (c == -1)
                break;

            switch (c) {
                case 'c':
                    cluster = optarg;
                    break;
                case 't':
                    timeout = parseNonNegativeDuration(optarg);
                    break;
                case 'h':
                    usage();
                    exit(0);
                case 'v':
                    logPolicy = "VERBOSE";
                    break;
                case 256:
                    logPolicy = optarg;
                    break;
                case '?':
                default:
                    // getopt_long already printed an error message.
                    usage();
                    exit(1);
            }
        }
    }

    void usage() {
        std::cout
            << "Writes a value to LogCabin. This isn't very useful on its own "
            << "but serves as a"
            << std::endl
            << "good starting point for more sophisticated LogCabin client "
            << "programs."
            << std::endl
            << std::endl
            << "This program is subject to change (it is not part of "
            << "LogCabin's stable API)."
            << std::endl
            << std::endl

            << "Usage: " << argv[0] << " [options]"
            << std::endl
            << std::endl

            << "Options:"
            << std::endl

            << "  -c <addresses>, --cluster=<addresses>  "
            << "Network addresses of the LogCabin"
            << std::endl
            << "                                         "
            << "servers, comma-separated"
            << std::endl
            << "                                         "
            << "[default: logcabin:5254]"
            << std::endl

            << "  -h, --help                     "
            << "Print this usage information"
            << std::endl

            << "  -t <time>, --timeout=<time>    "
            << "Set timeout for individual operations"
            << std::endl
            << "                                 "
            << "(0 means wait forever) [default: 0s]"
            << std::endl

            << "  -v, --verbose                  "
            << "Same as --verbosity=VERBOSE"
            << std::endl

            << "  --verbosity=<policy>           "
            << "Set which log messages are shown."
            << std::endl
            << "                                 "
            << "Comma-separated LEVEL or PATTERN@LEVEL rules."
            << std::endl
            << "                                 "
            << "Levels: SILENT ERROR WARNING NOTICE VERBOSE."
            << std::endl
            << "                                 "
            << "Patterns match filename prefixes or suffixes."
            << std::endl
            << "                                 "
            << "Example: Client@NOTICE,Test.cc@SILENT,VERBOSE."
            << std::endl;
    }

    int& argc;
    char**& argv;
    std::string cluster;
    std::string logPolicy;
    uint64_t timeout;
};

} // anonymous namespace

int 
main(int argc, char** argv)
{
    OptionParser options(argc, argv);
    LogCabin::Client::Debug::setLogPolicy(
        LogCabin::Client::Debug::logPolicyFromString(
            options.logPolicy));
    Cluster cluster(options.cluster);
    Tree tree = cluster.getTree();
    tree.setTimeout(options.timeout);
    
    tree.makeDirectoryEx("/KV_db");
    tree.setWorkingDirectoryEx("/KV_db");
    std::string working_dir = tree.getWorkingDirectory();
    assert(working_dir == "/KV_db");

    tree.writeEx("0xAAAAAAAA", "233233", 1);
    tree.writeEx("0xBBBBBBBB", "123456", 1);
    std::vector<std::string> key_list = std::move(tree.listDirectoryEx("./"));
    std::cout<<"Current keys:"<<std::endl;
    for (auto key: key_list) {
        std::cout<<key<<std::endl;
    }

    std::string contents = tree.readEx("0xAAAAAAAA", 1);
    std::cout<<"Read content: "<<contents<<std::endl;

    tree.setWorkingDirectoryEx("/");
    tree.removeDirectoryEx("/KV_db");

    std::cout<<"Test ends"<<std::endl;
    return 0;
}