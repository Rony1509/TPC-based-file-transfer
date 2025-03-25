#include <boost/asio.hpp>

#include <iostream>

#include <fstream>

#include <string>

#include <thread>



using boost::asio::ip::tcp;



void receiveFile(tcp::socket socket) {

    try {

        while (true) {

            // get file name

            std::string filename;

            boost::asio::streambuf buf;

            boost::asio::read_until(socket, buf, '\n');

            std::istream is(&buf);

            std::getline(is, filename);

            std::cout << "Receiving file: " << filename << std::endl;



            

            std::string custom_filename;

            std::cout << "Enter custom file name to save as (or press Enter to use the original name): ";

            std::getline(std::cin, custom_filename);

            if (custom_filename.empty()) {

                custom_filename = filename;    

            }



            

            size_t file_size = 0;

            boost::asio::read(socket, boost::asio::buffer(&file_size, sizeof(file_size)));

            std::cout << "File size: " << file_size << " bytes\n";



            

            std::ofstream file(custom_filename, std::ios::binary);

            if (!file) {

                std::cerr << "Error opening file: " << custom_filename << std::endl;

                return;

            }



            


            size_t bytes_received = 0;

            char buffer[4096];

            while (bytes_received < file_size) {

                size_t len = socket.read_some(boost::asio::buffer(buffer, std::min(sizeof(buffer), file_size - bytes_received)));

                file.write(buffer, len);

                bytes_received += len;

                std::cout << "Received " << bytes_received << " of " << file_size << " bytes\n";

            }



            std::cout << "File transfer complete. Total bytes received: " << bytes_received << "\n";

        }

    } catch (std::exception& e) {

        std::cerr << "Error: " << e.what() << std::endl;

    }

}



int main() {

    try {

        boost::asio::io_service io_service;

        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 12345));

        std::cout << "Server listening on port 12345..\n";



        while (true) {

            

            tcp::socket socket(io_service);

            acceptor.accept(socket);



            

            std::thread client_thread(receiveFile, std::move(socket));

            client_thread.detach(); 

        }



    } catch (std::exception& e) {

        std::cerr << "Error: " << e.what() << std::endl;

    }



    return 0;

}
