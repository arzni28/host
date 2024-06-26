#include <uwebsockets/App.h>
#pragma comment (lib, "zlib.lib")
#pragma comment (lib, "uv.lib")
#pragma comment (lib, "uSockets.lib")
using namespace std;


const string SET_NAME = "SET_NAME::";
const string DIRECT = "DIRECT::";

bool isSetNameCommand(string_view message) {
    return message.find(SET_NAME) == 0;
}

string parseName(string_view message)
{
    return string(message.substr(SET_NAME.length()));
}

string parseRecieverId(string_view message) {
    string_view rest = message.substr(DIRECT.length());
    int pos = rest.find("::");
    string_view id = rest.substr(0, pos);
    return string(id);
}

string parseDirectMessage(string_view message) {
    string_view rest = message.substr(DIRECT.length());
    int pos = rest.find("::");
    string_view text = rest.substr(pos + 2);
    return string(text);
}

bool isDirectCommand(string_view message) {
    return message.find("DIRECT::") == 0;
}

int main() {
    /* ws->getUserData returns one of these */
    struct PerSocketData {
        /* Fill with user data */
        int user_id;
        string name;

    };

    int last_user_id = 10;

    /* Keep in mind that uWS::SSLApp({options}) is the same as uWS::App() when compiled without SSL support.
     * You may swap to using uWS:App() if you don't need SSL */
    uWS::App()
        .ws<PerSocketData>("/*", 
        {
            /* Settings */
            .compression = uWS::CompressOptions(uWS::DEDICATED_COMPRESSOR_4KB | uWS::DEDICATED_DECOMPRESSOR),
            .maxPayloadLength = 100 * 1024 * 1024,
            .idleTimeout = 16,
            .maxBackpressure = 100 * 1024 * 1024,
            .closeOnBackpressureLimit = false,
            .resetIdleTimeoutOnSend = false,
            .sendPingsAutomatically = true,

            /* Handlers */
            .upgrade = nullptr,
            .open = [&last_user_id](auto* connection) {
                cout << "New Connection created\n";
                PerSocketData* userData = (PerSocketData*)connection->getUserData();
                userData->user_id = last_user_id++;
                userData->name = "UNNAMED";

                connection->subscribe("Broadcast");
                connection->subscribe("user#" + to_string(userData->user_id));
                /* Open event here, you may access ws->getUserData() which points to a PerSocketData struct */

            },
            .message = [](auto* connection, string_view message, uWS::OpCode opCode) {
                cout << "New message recieved\n"<<message<<"\n";
                PerSocketData* userData = (PerSocketData*)connection->getUserData();
                if (isSetNameCommand(message))
                {
                    cout << "User set their name\n";
                    userData->name = parseName(message);
                }
                if (isDirectCommand(message))
                {
                    string id = parseRecieverId(message);
                    string text = parseDirectMessage(message);
                    cout << "User sent direct message\n";
                    connection->publish("user#" + id, text);
                }
            },
            .dropped = [](auto*/*ws*/, string_view /*message*/, uWS::OpCode /*opCode*/) {
                /* A message was dropped due to set maxBackpressure and closeOnBackpressureLimit limit */
            },
            .drain = [](auto*/*ws*/) {
                /* Check ws->getBufferedAmount() here */
            },
            .ping = [](auto*/*ws*/, string_view) {
                /* Not implemented yet */
            },
            .pong = [](auto*/*ws*/, string_view) {
                /* Not implemented yet */
            },
            .close = [](auto*/*ws*/, int /*code*/, string_view /*message*/) {
                cout << "Connection closed\n";
                /* You may access ws->getUserData() here */
            }
        }).listen(8080, [](auto* listen_socket) {
                if (listen_socket) {
                    cout << "Listening on port " << 8080 << endl;
                }
        }).run();
}