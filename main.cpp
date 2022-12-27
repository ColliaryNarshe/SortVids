#include <algorithm> // for transform
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>
namespace fs = std::filesystem;

std::set<fs::path> getFilenames(std::string_view dir);

// Checks filenames against a vector of keywords
bool checkFilenames( const fs::path& filename, const std::pair< fs::path, std::vector<std::string> >& pair );

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
std::map< fs::path, std::vector<std::string> > parseLines(const std::vector<std::string>& lines);


void printFilenames(const std::vector<std::pair<fs::path, fs::path>>& filenamePairs);


void renameFilenames(const std::vector<std::pair<fs::path, fs::path>>& filenamePairs);


// Creates list of files to rename: std::pair<file, new location>
bool getFilesToRename(std::vector<std::string>& lineVector, 
                      std::vector< std::pair<fs::path, fs::path> >& newFilenames);


int main(){
    // Extract lines from data file:
    std::vector<std::string> lineVector = getFileLines(".\\sortvidKeywords.txt");
    std::cout << '\n';

    // Get list of matching files
    std::vector< std::pair<fs::path, fs::path> > newFilenames{};
    if (!getFilesToRename(lineVector, newFilenames))
        return 0;

    printFilenames(newFilenames);
    std::cout << "\nWould you like to move the file(s)? Enter q to quit.\n";
    std::string query{};
    getline(std::cin, query);
    if (query == "q")
        return 0;
    
    renameFilenames(newFilenames);
    std::cout << "File(s) moved.\n\n";

    return 0;
}



bool checkFilenames( const fs::path& filename, const std::pair< fs::path, std::vector<std::string> >& pair )
{
    // check if file exists and omit destination from match check:
    if ((pair.first.filename() == filename.filename()))
        return false;
    
    // Transform filename to lowercase
    std::string fileLower{ filename.filename().string() };
    transform(fileLower.begin(), fileLower.end(), fileLower.begin(), ::tolower);

    for (std::string word: pair.second)
    {
        transform(word.begin(), word.end(), word.begin(), ::tolower);
        if ( fileLower.find(word) == std::string::npos)
            return false;
    }
    return true;
}


std::set<fs::path> getFilenames(std::string_view dir)
{
    std::set<fs::path> filePaths{};

    for ( const auto& path: fs::directory_iterator(dir) )
        {
            filePaths.insert(path);
        }
    return filePaths;
}



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



std::map< fs::path, std::vector<std::string> > parseLines(const std::vector<std::string>& lines)
{
    std::map< fs::path, std::vector<std::string> > fileMap{};

    for(auto line: lines)
    {
        std::vector<std::string> keywords{ splitString(line, ",", true) };
        fs::path showPath{keywords[0]};
        keywords.erase(keywords.begin());
        std::pair<fs::path, std::vector<std::string> > combined{showPath, keywords};
        fileMap.insert( combined );
    }
    return fileMap;
}



void printFilenames(const std::vector<std::pair<fs::path, fs::path>>& filenamePairs)
{
    for (auto& pair: filenamePairs)
    {
        std::cout << pair.first.filename() 
            << "\n\t-----> " << pair.second.parent_path().filename() << '\n';
    }
}



void renameFilenames(const std::vector<std::pair<fs::path, fs::path>>& filenamePairs)
{
    for (auto& pair: filenamePairs)
    {
        try
            { fs::rename(pair.first, pair.second); }
        catch(std::exception& e)
            { std::cout << e.what() << '\n'; }
    }
}


void checkDestinations(std::map< fs::path, std::vector<std::string> >& fileMap)
{
    bool badPath{};

    for (std::pair pair: fileMap)
    {
        // check if file exists
        if( !fs::exists(pair.first) )
        {
            std::cout << "Path doesn't exist: " << pair.first << "\n\n";
            fileMap.erase(pair.first);
            badPath = true;
        }

    }
    if (badPath)
    {
        std::cout << "Press any key to continue.\n";
        std::cin.ignore();
    }
}


bool getFilesToRename(std::vector<std::string>& lineVector, 
                      std::vector< std::pair<fs::path, fs::path> >& newFilenames)
{
    // Get the filePath to directory of files
    std::string filePath = lineVector[0];
    lineVector.erase(lineVector.begin());
    filePath = filePath.substr(7);

    // Get list of filenames in file origin directory
    std::set<fs::path> filenames = getFilenames(filePath);

    // Create map with destination path and keywords
    std::map< fs::path, std::vector<std::string> > fileMap{parseLines(lineVector)};
    checkDestinations(fileMap);

    int16_t count{};

    for (const fs::path& filename: filenames) 
    {
        // Loop through map of files
        for (auto pair: fileMap)
        {
            // Check for matches
            if ( checkFilenames(filename, pair ) )
            {
                ++count;
                fs::path new_filename{pair.first};
                new_filename /= filename.filename();
                newFilenames.push_back(std::pair(filename, new_filename));
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

