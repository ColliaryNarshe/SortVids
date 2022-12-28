#include <algorithm> // for transform
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>
namespace fs = std::filesystem;

struct Data
{
    std::vector<std::string> fileLines{};
    fs::path searchDirectory{};
    std::set<fs::path> allFiles{};
};

struct Keywords
{
    fs::path destination{};
    std::vector<std::string> keywords{};

};

struct Match
{
    fs::path oldPath{};
    fs::path newPath{};
};


std::set<fs::path> getFilenames(fs::path dir);

// Checks filenames against a vector of keywords
bool checkFilenames( const fs::path& filename, const Keywords& kw );

// Returns a vector with lines from a text file.
std::vector<std::string> getFileLines(const std::string& path);

// Remove spaces from both ends of a string
std::string removeSpace(std::string s);

// Returns a vector of a string split along a delimiter
std::vector<std::string> splitString(std::string_view str, 
                                     std::string_view delimiter, 
                                     bool removeSpaces = false);

// Takes vector string lines, each with path to show dir and keywords to search for
// Returns a map with they dir and keywords divided. map<dir,keywords>
std::vector<Keywords> parseLines(const std::vector<std::string>& lines);

void printFilenames(const std::vector<Match>& filenamePairs);

void renameFilenames(const std::vector<Match>& filenamePairs);

// Creates list of files to rename: std::pair<file, new location>
bool getFilesToRename(Data& data, Keywords& keywords,
                      std::vector<Match>& matches);

// Creates the data file with input from user
void createDataFile();


int main(int argc, char* argv[]){
    std::string filePath{"sortvidKeywords.txt"};
    
    // Use command line to change datafile name
    if (argc > 1)
        filePath = argv[1];
    
    // Option to create data file if not found
    else if (!fs::exists(filePath))
    {
        std::cout << "Data file not found: " << filePath << '\n';
        std::cout << "Would you like to create one? y/n\n";
        std::string query{};
        getline(std::cin, query);
            if (query == "y")
                createDataFile();
        return 0;
    }

    // structs
    Data data{};
    Keywords keywords{};

    // Extract lines from data file:
    data.fileLines = getFileLines(filePath);
    std::cout << '\n';

    // Get list of matching files
    std::vector<Match> matches{};
    if (!getFilesToRename(data, keywords, matches))
        return 0;

    // Print what will be moved and option to quit
    printFilenames(matches);
    std::cout << "\nWould you like to move the file(s)? Enter q to quit.\n";
    std::string query{};
    getline(std::cin, query);
    if (query == "q")
        return 0;
    
    // Move the files
    renameFilenames(matches);
    std::cout << "File(s) moved.\n";

    return 0;
}



bool checkFilenames( const fs::path& filename, const Keywords& kw )
{
    // check if file exists and omit destination from match check:
    if ((kw.destination.filename() == filename.filename()))
        return false;
    
    // Transform filename to lowercase
    std::string fileLower{ filename.filename().string() };
    transform(fileLower.begin(), fileLower.end(), fileLower.begin(), ::tolower);

    for (std::string word: kw.keywords)
    {
        transform(word.begin(), word.end(), word.begin(), ::tolower);
        if ( fileLower.find(word) == std::string::npos)
            return false;
    }
    return true;
}


std::set<fs::path> getFilenames(fs::path dir)
{
    std::set<fs::path> filePaths{};

    for ( const auto& path: fs::directory_iterator(dir) )
        {
            filePaths.insert(path);
        }
    return filePaths;
}

// =========Functions===================================

// Returns a vector with lines from a text file.
std::vector<std::string> getFileLines(const std::string& path)
{
    std::vector<std::string> lines{};
    std::ifstream fileData{};
    
    fileData.open(path, std::ios::in);
    if ( fileData.is_open() )
    {
        std::string line{};
        while(getline(fileData, line))
            if (line != "")
                lines.push_back(line);

        fileData.close();
    }
    else
    {
        std::cout << "Error opening file: \"" << path << "\"\n";
    }

    return lines;
}


std::string removeSpace(std::string s)
{
    s.erase(s.find_last_not_of(' ') + 1);
    s.erase( 0, s.find_first_not_of(' ') );
    return s;
}


std::vector<std::string> splitString(std::string_view str, 
                                     std::string_view delimiter, 
                                     bool removeSpaces)
{
    std::vector<std::string> splitLines{};
    std::string s{};
    size_t idx{};
    size_t prev{};

    while( ( idx = str.find(delimiter, prev) ) != std::string::npos )
    {
        s = str.substr(prev, idx - prev);
        if (removeSpaces)
            s = removeSpace(s);
        splitLines.push_back( s );
        prev = idx + delimiter.length();
    }

    // Add last substring
    s = str.substr(prev);
    if (removeSpaces)
            s = removeSpace(s);
    splitLines.push_back( s );

    return splitLines;
}


std::vector<Keywords> parseLines(const std::vector<std::string>& lines)
{
    std::vector<Keywords> keywordVector{};

    for(auto line: lines)
    {
        std::vector<std::string> keywords{ splitString(line, ",", true) };
        fs::path destPath{keywords[0]};
        keywords.erase(keywords.begin());
        keywordVector.push_back( {{destPath}, {keywords}} );
    }
    return keywordVector;
}


void printFilenames(const std::vector<Match>& filenamePairs)
{
    for (auto& match: filenamePairs)
    {
        std::cout << match.oldPath.filename() 
            << "\n\t-----> " << match.newPath.parent_path().filename() << '\n';
    }
}


void renameFilenames(const std::vector<Match>& filenamePairs)
{
    for (auto& match: filenamePairs)
    {
        try
            { fs::rename(match.oldPath, match.newPath); }
        catch(std::exception& e)
            { std::cout << "Could not rename file: " << e.what() << '\n'; }
    }
}


void checkDestinations(std::vector<Keywords>& keywords)
{
    bool badPath{};
    int16_t idx{};
    std::vector<Keywords> keywords_copy{keywords};

    for (auto& keys: keywords_copy)
    {
        ++idx;
        // check if file exists
        if( !fs::exists(keys.destination) )
        {
            std::cout << "Path doesn't exist: " << keys.destination << '\n';
            keywords.erase(keywords.begin() + idx);
            badPath = true;
        }
    }
    if (badPath)
    {
        std::cout << "Press any key to continue.\n";
        std::cin.ignore();
    }
}


bool getFilesToRename(Data& data, Keywords& keywords,
                      std::vector<Match>& matches)
{
    // Get the filePath to directory of files
    std::string filePath = data.fileLines[0];
    data.fileLines.erase(data.fileLines.begin());
    data.searchDirectory = filePath.substr(7);

    // Get list of filenames in file origin directory
    data.allFiles = getFilenames(data.searchDirectory);

    // Create map with destination path and keywords
    std::vector<Keywords> keywordVector{parseLines(data.fileLines)};
    checkDestinations(keywordVector);

    int16_t count{};

    for (const fs::path& filename: data.allFiles) 
    {
        // Loop through map of files
        for (auto kw: keywordVector)
        {
            // Check for matches
            if ( checkFilenames(filename, kw ) )
            {
                ++count;
                fs::path new_filename{kw.destination};
                new_filename /= filename.filename();
                Match x{{filename},{new_filename}};
                matches.push_back(x);
            }
        }
    }
    if (!count)
    {
        std::cout << "No files found.\n\n";
        return false;
    } else
        return true;
}


void createDataFile()
{
    std::string path{"sortvidKeywords.txt"};
    if (fs::exists(path))
        path = "sortvidKeywords1.txt";

    std::ofstream fileData{path};

    fileData << "files: .\\\n";

    std::string query{};
    while (true)
    {
        std::cout << "Enter a destination folder or q to quit:\n";
        getline(std::cin, query);
        if (query == "q")
            break;
        fileData << query << ", ";
        std::cout << "Enter keywords separated by comas:\n";
        getline(std::cin, query);
        fileData << query << '\n';
    }

    fileData.close();
}
