#include <SFML/Graphics.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <thread>

using boost::asio::ip::tcp;

// it will set progressbar
void sendFileWithProgress(tcp::socket& socket, const std::string& filename, sf::RenderWindow& window, sf::Font& font) {

    // Progress Bar making

    sf::RectangleShape progressBar(sf::Vector2f(400, 30));
    progressBar.setFillColor(sf::Color::Green);
    progressBar.setPosition(50, 80);

    sf::RectangleShape progressBarBackground(sf::Vector2f(400, 30));
    progressBarBackground.setFillColor(sf::Color(100, 100, 100));
    progressBarBackground.setPosition(50, 80);

    // Progress Text

    sf::Text progressText("0%", font, 20);
    progressText.setPosition(230, 120);
    progressText.setFillColor(sf::Color::White);

    // it will take file name

    boost::asio::write(socket, boost::asio::buffer(filename + "\n"));

    // reading file

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    // file size

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // give file size

    boost::asio::write(socket, boost::asio::buffer(&file_size, sizeof(file_size)));

    // Reset progress bar 
    size_t bytes_sent = 0;
    progressBar.setSize(sf::Vector2f(0, 30));  
    progressText.setString("0%");

    // Send file in chunks

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer))) {
        boost::asio::write(socket, boost::asio::buffer(buffer, file.gcount()));
        bytes_sent += file.gcount();

        // Update progress
        float progress = static_cast<float>(bytes_sent) / file_size;
        progressBar.setSize(sf::Vector2f(400 * progress, 30));
        progressText.setString(std::to_string(static_cast<int>(progress * 100)) + "%");

        // Update window
        window.clear();
        window.draw(progressBarBackground);
        window.draw(progressBar);
        window.draw(progressText);
        window.display();
    }

    // Send remaining bytes (if any)
    if (file.gcount() > 0) {
        boost::asio::write(socket, boost::asio::buffer(buffer, file.gcount()));
        bytes_sent += file.gcount();
    }

    // Final progress 

    float final_progress = static_cast<float>(bytes_sent) / file_size;
    progressBar.setSize(sf::Vector2f(400 * final_progress, 30));
    progressText.setString("100%");

    // Final update to show 100%

    window.clear();
    window.draw(progressBarBackground);
    window.draw(progressBar);
    window.draw(progressText);
    window.display();

    std::cout << "File transfer complete: " << filename << "\n";
}

int main() {
    std::string server_address, port;
    std::cout << "Enter server address: ";
    std::cin >> server_address;
    std::cout << "Enter server port: ";
    std::cin >> port;

    // SFML Window

    sf::RenderWindow window(sf::VideoMode(500, 400), "File Transfer");

    // open front

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Error loading font\n";
        return 1;
    }

    
    sf::Text fileNameText("Enter file name:", font, 20);

    fileNameText.setPosition(50, 20);
    fileNameText.setFillColor(sf::Color::White);

    sf::RectangleShape inputBox(sf::Vector2f(300, 30));
    inputBox.setFillColor(sf::Color(200, 200, 200));
    inputBox.setPosition(150, 20);

    sf::Text inputText("", font, 20);
    inputText.setPosition(160, 25);
    inputText.setFillColor(sf::Color::Black);

    // Create Send Button f
    sf::RectangleShape sendButton(sf::Vector2f(100, 40));
    sendButton.setFillColor(sf::Color::Blue);
    sendButton.setPosition(200, 70);

    sf::Text sendButtonText("Send", font, 20);
    sendButtonText.setPosition(225, 80);
    sendButtonText.setFillColor(sf::Color::White);

    try {
        boost::asio::io_service io_service;
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(server_address, port);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::socket socket(io_service);
        boost::asio::connect(socket, endpoint_iterator);

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    window.close();

                // file name input

                if (event.type == sf::Event::TextEntered) {
                    if (event.text.unicode < 128 && event.text.unicode != 13) { // Check for enter key (13)
                        inputText.setString(inputText.getString() + static_cast<char>(event.text.unicode));
                    }
                    if (event.text.unicode == 8) { 
                        std::string currentText = inputText.getString();
                        if (!currentText.empty()) {
                            currentText.pop_back();
                            inputText.setString(currentText);
                        }
                    }
                }

                // click button
                
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    if (sendButton.getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y)) {
                        std::string filename = inputText.getString();
                        if (!filename.empty()) {
                            sendFileWithProgress(socket, filename, window, font);
                            inputText.setString("");  // Clear input for next file
                        }
                    }
                }
            }

            window.clear();
            window.draw(fileNameText);
            window.draw(inputBox);
            window.draw(inputText);
            window.draw(sendButton);
            window.draw(sendButtonText);
            window.display();
        }
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
