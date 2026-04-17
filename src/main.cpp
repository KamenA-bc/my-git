#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <zlib.h>
#include <stdexcept>
#include <cstring>
#include <sstream>
#include <vector>

std::string decompress_zlib(const std::string& compressed_data);

int main(int argc, char *argv[])
{
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here!\n";
    
    if (argc < 2) {
        std::cerr << "No command provided.\n";
        return EXIT_FAILURE;
    }
    
    std::string command = argv[1];
    std::string blob = argv[3];

    

    if (command == "init") {
        try {
            std::filesystem::create_directory(".git");
            std::filesystem::create_directory(".git/objects");
            std::filesystem::create_directory(".git/refs");
    
            std::ofstream headFile(".git/HEAD");
            if (headFile.is_open()) {
                headFile << "ref: refs/heads/main\n";
                headFile.close();
            } else {
                std::cerr << "Failed to create .git/HEAD file.\n";
                return EXIT_FAILURE;
            }
    
            std::cout << "Initialized git directory\n";
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }
    else if(command == "cat-file")
    {
        std::string blobDirectory = blob.substr(0, 2); 
        
        std::string blobFileName = blob.substr(2);

        std::string fullPath = ".git/objects/" + blobDirectory + "/" + blobFileName;
        std::string blobObject;

        std::ifstream file (fullPath, std::ios::binary | std::ios::ate);
        if(!file.is_open())
        {
            std::cerr << "Error opening file\n";
            return EXIT_FAILURE;
        }
        std::streamsize size = file.tellg();

        file.seekg(0, std::ios::beg);

        std::string buffer(size, '\0');

        if (file.read(buffer.data(), size)) 
        {
            blobObject = buffer;
        } 
        else 
        {
            throw std::runtime_error("Failed to read the file entirely.");
        }
        std::string decompressedBlob = decompress_zlib(blobObject);
        std::stringstream ss(decompressedBlob);
        std::string blobBody;
        size_t null_pos = decompressedBlob.find('\0');

            if (null_pos != std::string::npos) 
            {
            std::string blobBody = decompressedBlob.substr(null_pos + 1);

            std::cout << blobBody;
            }

    } 
    else 
    {
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }


    
    return EXIT_SUCCESS;
}

std::string decompress_zlib(const std::string& compressed_data) {
    // 1. Initialize the zlib stream structure
    z_stream zs;
    std::memset(&zs, 0, sizeof(zs));

    // inflateInit initializes the decompression state. 
    // It returns Z_OK if successful.
    if (inflateInit(&zs) != Z_OK) {
        throw std::runtime_error("inflateInit failed while decompressing.");
    }

    // 2. Point zlib to our compressed input data
    // next_in expects a pointer to the start of the data
    // avail_in tells it exactly how many bytes it has to read
    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed_data.data()));
    zs.avail_in = compressed_data.size();

    int ret;
    char outbuffer[32768]; // A 32KB buffer to hold chunks of decompressed data
    std::string decompressed_string;

    // 3. Loop to decompress the data in chunks
    do {
        // Reset the output pointer and available output size for this chunk
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        // inflate does the actual decompression work.
        // It reads from next_in and writes to next_out.
        ret = inflate(&zs, Z_NO_FLUSH);

        // Check for catastrophic decompression errors
        if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&zs);
            throw std::runtime_error("Exception during zlib decompression (corrupt data).");
        }

        // Calculate how many bytes were newly decompressed in this loop iteration
        // and append them to our final string.
        int bytes_decompressed = sizeof(outbuffer) - zs.avail_out;
        decompressed_string.append(outbuffer, bytes_decompressed);

    } while (ret == Z_OK); // Z_OK means there's more data to process

    // 4. Clean up the zlib stream to free memory
    inflateEnd(&zs);

    // If the loop finished but didn't reach the end of the stream, something went wrong
    if (ret != Z_STREAM_END) {
        throw std::runtime_error("Exception during zlib decompression (incomplete stream).");
    }

    return decompressed_string;
}