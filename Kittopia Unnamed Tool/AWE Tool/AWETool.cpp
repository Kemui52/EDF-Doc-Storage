// AWETool.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <filesystem>

class AWEFile
{
public:
    AWEFile( std::string path )
    {
        //load file.
        std::ifstream fileStream;
        fileStream.open(path, std::ios_base::binary );

        if( fileStream.is_open() ) //Success.
        {
            //Read file.
            char buf[4];
            fileStream.read( buf, 4 );

            if( strcmp( buf, "AWBE" ) != 0 )
            {
                isLoaded = false;
                return;
            }

            //This is an AWE file.
            fileStream.seekg( 0x8, std::ios_base::beg );
            fileStream.read( buf, 4 );

            int count = 0;
            memcpy( &count, buf, sizeof( int ) );

            //Read starts
            int str_ptr_start = 0;
            fileStream.read( buf, 4 );
            memcpy( &str_ptr_start, buf, sizeof( int ) );

            int file_num_list_start = 0;
            fileStream.read( buf, 4 );
            memcpy( &file_num_list_start, buf, sizeof( int ) );

            //read names
            std::vector< std::string > nameList;
            for( int i = 0; i < count; ++i )
            {
                int offset = str_ptr_start + (i * 4);
                int string_offset = 0;

                fileStream.seekg( offset, std::ios_base::beg );
                fileStream.read( buf, 4 );

                memcpy( &string_offset, buf, sizeof( int ) );

                fileStream.seekg( offset + string_offset, std::ios_base::beg );

                std::string str;
                std::getline( fileStream, str, '\0' );

                nameList.push_back( str );
            }

            //read indices
            std::vector< int > indices;
            for( int i = 0; i < count; ++i )
            {
                int offset = file_num_list_start + (i * 2);
                short index = 0;

                fileStream.seekg( offset, std::ios_base::beg );
                fileStream.read( buf, 2 );

                memcpy( &index, buf, sizeof( short ) );
                if( index > count )
                    std::cout << "Possible error?";

                indices.push_back( index );

                if( nameMap.find( index ) != nameMap.end() )
                    std::cout << "Possible error?";

                nameMap[index] = nameList[i];
            }

            //Close
            fileStream.close();
            isLoaded = true;
        }
        else
        {
            //Todo: Some failure behaviour.
            isLoaded = false;
        }
    };

    bool isLoaded = false;
    std::map< int, std::string > nameMap;
};

struct AWBSubFile
{
    int id;
    int start;
    int size;
};

class AWBFile
{
public:
    AWBFile( std::string path )
    {
        //load file.
        fileStream.open( path, std::ios_base::binary );

        if( fileStream.is_open() ) //Success.
        {
            //Read file.
            char buf[4];
            fileStream.read( buf, 4 );

            //check magic
            if( strcmp( buf, "AFS2" ) != 0 )
            {
                isLoaded = false;
                return;
            }

            //Read header
            int version, datasize, wav_allignment;

            fileStream.read( buf, 4 );

            version = buf[0];
            datasize = buf[1];
            wav_allignment = buf[2];

            int count = 0;
            fileStream.read( buf, 4 );
            memcpy( &count, buf, sizeof( int ) );

            int ofs_alignment = 0;
            fileStream.read( buf, 4 );
            memcpy( &ofs_alignment, buf, sizeof( int ) );

            //Begin reading start positions.
            for( int i = 0; i < count; ++i )
            {
                int offset = 0x10 + (i * wav_allignment);

                fileStream.seekg( offset, std::ios_base::beg );
                fileStream.read( buf, wav_allignment );

                int songID;
                memcpy( &songID, buf, wav_allignment );

                offset = 0x10 + (count * wav_allignment);
                offset += i * datasize;

                fileStream.seekg( offset, std::ios_base::beg );
                int filestart, fileend = 0;

                fileStream.read( buf, datasize );
                memcpy( &filestart, buf, datasize );

                fileStream.read( buf, datasize );
                memcpy( &fileend, buf, datasize );

                filestart += (filestart % ofs_alignment) ? ofs_alignment - (filestart % ofs_alignment) : 0;
                fileend += (fileend % ofs_alignment) ? ofs_alignment - (fileend % ofs_alignment) : 0;
                
                int subfile_size = fileend - filestart;

                AWBSubFile file;
                file.id = songID;
                file.start = filestart;
                file.size = subfile_size;

                files.push_back( file );
            }

            isLoaded = true;
        }
        else
        {
            //Todo: Some failure behaviour.
            isLoaded = false;
        }
    }

    //cleanup.
    ~AWBFile()
    {
        if( fileStream.is_open() )
            fileStream.close();

        files.clear();
    }

    //Extract single file by index
    void ExtractFile( int index, AWEFile& file, std::string outputPath )
    {
        if( !(std::filesystem::exists( outputPath ) && std::filesystem::is_directory( outputPath )) )
            return;

        char *buffer;
        buffer = new char[ files[index].size ];
        fileStream.seekg( files[index].start, std::ios_base::beg );
        fileStream.read( buffer, files[index].size );

        std::string filePath = (std::filesystem::path( outputPath ) / "").generic_string();

        //Create file.
        std::string fileName;
        fileName = file.nameMap[ files[index].id ];

        std::ofstream outputFile( filePath + fileName + ".hca", std::ios::out | std::ios::binary | std::ios::app );
        outputFile.write( buffer, files[index].size );
        outputFile.close();

        delete[] buffer;
    }

    void ExtractAll( AWEFile& file, std::string outputPath )
    {
        for( int i = 0; i < files.size(); ++i )
        {
            ExtractFile( i, file, outputPath );

            std::cout << std::to_string( i ) + "/" + std::to_string( files.size() ) + "\n"; //verbose output.
        }
    }

    void Close()
    {
        if( fileStream.is_open() )
            fileStream.close();

        files.clear();

        isLoaded = false;
    }

    bool isLoaded = false;
    std::ifstream fileStream;

    std::vector< AWBSubFile > files;
};

int main()
{
    //std::cout << "Input file:";
    //std::string file;
    //std::cin >> file;
    
    //AWEFile awe( file );
    //AWEFile awe( "TIKYUU6_VOICE_DLC2.EN_LIST.AWE" );
    //AWBFile awb( "TIKYUU6_VOICE_DLC2.EN.AWB" );

    //awb.ExtractAll( awe, "OUT/" );

    //awb.Close();

    std::cout << "Input AWE file:";
    std::string file;
    std::cin >> file;

    AWEFile awe( file );
    if( awe.isLoaded == false )
    {
        std::cout << "File failed to load, aborting.\n";
        return 0;
    }

    std::cout << "Input AWB file:";
    file.clear();

    std::cin >> file;
    AWBFile awb( file );
    if( awb.isLoaded == false )
    {
        std::cout << "File failed to load, aborting.\n";
        return 0;
    }

    //std::cout << "Input operation ( 0 extract single file, 1: extract all ):";
    std::cout << "Files loaded. Select output directory:";
    std::cin >> file;

    std::cout << "Attempting extract.\n";

    awb.ExtractAll( awe, file );
    awb.Close();

    return 1;

    //search
    //while( true )
    //{
    //    std::cout << "Search: ";
    //    std::string name;
    //    std::cin >> name;
    //
    //    int key = -1;
    //    for( auto& i : awe.nameMap )
    //    {
    //        if( i.second == name )
    //        {
    //            key = i.first;
    //            break;
    //        }
    //    }
    //
    //    if( key != -1 )
    //        std::cout << "ID is: " + std::to_string(key) + "\n";
    //    else
    //        std::cout << "Not found.\n";
    //}

    //std::cout << "Hello World!\n";
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
