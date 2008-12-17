//  Copyright (C) 2007 Robert Kooima
//
//  THUMB is free software; you can redistribute it and/or modify it under
//  the terms of  the GNU General Public License as  published by the Free
//  Software  Foundation;  either version 2  of the  License,  or (at your
//  option) any later version.
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.

#include <SDL.h>
#include <iostream>

#include <string.h>

#include "util.hpp"
#include "matrix.hpp"
#include "tracker.hpp"
#include "ogl-opengl.hpp"
#include "app-data.hpp"
#include "app-conf.hpp"
#include "app-glob.hpp"
#include "app-user.hpp"
#include "app-host.hpp"
#include "app-perf.hpp"
#include "app-prog.hpp"

#define JIFFY (1000 / 60)

//-----------------------------------------------------------------------------

static unsigned long lookup(const char *hostname)
{
    struct hostent *H;
    struct in_addr  A;

    if ((H = gethostbyname(hostname)) == 0)
        throw app::host_error(hostname);

    memcpy(&A.s_addr, H->h_addr_list[0], H->h_length);

    return A.s_addr;
}

//-----------------------------------------------------------------------------

void app::host::fork_client(const char *name, const char *addr)
{
#ifndef _WIN32 // W32 HACK
    const char *args[4];
    char line[256];

    if ((fork() == 0))
    {
        std::string dir = ::conf->get_s("exec_dir");

        sprintf(line, "cd %s; ./thumb %s", dir.c_str(), name);

        // Allocate and build the client's ssh command line.

        args[0] = "ssh";
        args[1] = addr;
        args[2] = line;
        args[3] = NULL;

        execvp("ssh", (char * const *) args);

        exit(0);
    }
#endif
}

//-----------------------------------------------------------------------------

SOCKET app::host::init_socket(int port)
{
    int sd = INVALID_SOCKET;

    if (port)
    {
        socklen_t  addresslen = sizeof (sockaddr_t);
        sockaddr_t address;

        address.sin_family      = AF_INET;
        address.sin_port        = htons(port);
        address.sin_addr.s_addr = INADDR_ANY;

        // Create a socket, set no-delay, bind the port, and listen.

        if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
            throw std::runtime_error(strerror(sock_errno));
        
        nodelay(sd);

        while (bind(sd, (struct sockaddr *) &address, addresslen) < 0)
            if (sock_errno == EINVAL)
            {
                std::cerr << "Waiting for port expiration" << std::endl;
                usleep(250000);
            }
            else throw std::runtime_error(strerror(sock_errno));

        listen(sd, 16);
    }
    return sd;
}

void app::host::init_listen(app::node node)
{
    client_cd = init_socket(get_attr_d(node, "port"));
    script_cd = init_socket(DEFAULT_SCRIPT_PORT);
}

void app::host::poll_listen()
{
    // NOTE: This must not occur between a host::send/host::recv pair.

    struct timeval tv = { 0, 0 };

    fd_set fds;
        
    FD_ZERO(&fds);

    if (client_cd != INVALID_SOCKET) FD_SET(client_cd, &fds);
    if (script_cd != INVALID_SOCKET) FD_SET(script_cd, &fds);

    int m = std::max(client_cd, script_cd);

    // Check for an incomming client connection.

    if (int n = select(m + 1, &fds, NULL, NULL, &tv))
    {
        if (n < 0)
        {
            if (sock_errno != EINTR)
                throw app::sock_error("select");
        }
        else
        {
            // Accept any client connection.

            if (client_cd != INVALID_SOCKET && FD_ISSET(client_cd, &fds))
            {
                if (int sd = accept(client_cd, 0, 0))
                {
                    if (sd < 0)
                        throw app::sock_error("accept");
                    else
                    {
                        nodelay(sd);
                        client_sd.push_back(sd);
                    }
                }
            }

            // Accept any script connection.

            if (script_cd != INVALID_SOCKET && FD_ISSET(script_cd, &fds))
            {
                if (int sd = accept(script_cd, 0, 0))
                {
                    if (sd < 0)
                        throw app::sock_error("accept");
                    else
                    {
                        nodelay(sd);
                        script_sd.push_back(sd);
                    }
                }
            }
        }
    }
}

void app::host::fini_listen()
{
    if (client_cd != INVALID_SOCKET) ::close(client_cd);
    if (script_cd != INVALID_SOCKET) ::close(script_cd);

    client_cd = INVALID_SOCKET;
    script_cd = INVALID_SOCKET;
}

void app::host::poll_script()
{
    struct timeval tv = { 0, 0 };

    fd_set fds;
    int m = 0;

    // Initialize the socket descriptor set with all connected sockets.

    FD_ZERO(&fds);

    for (SOCKET_i i = script_sd.begin(); i != script_sd.end(); ++i)
    {
        FD_SET(*i, &fds);
        m = std::max(m, *i);
    }

    // Check for activity on all sockets.

    if (int n = select(m + 1, &fds, NULL, NULL, &tv))
    {
        if (n < 0)
        {
            if (sock_errno != EINTR)
                throw std::runtime_error(strerror(sock_errno));
        }
        else
        {
            for (SOCKET_i t, i = script_sd.begin(); i != script_sd.end(); i = t)
            {
                // Step lightly in case *i is removed from the vector.

                t = i;
                t++;

                // Check for and handle script input.

                if (FD_ISSET(*i, &fds))
                {
                    char ibuf[256];
                    char obuf[256];

                    ssize_t sz;

                    memset(ibuf, 256, 0);
                    memset(obuf, 256, 0);

                    if ((sz = ::recv(*i, ibuf, 256, 0)) <= 0)
                    {
                        ::close(*i);
                        script_sd.erase(i);
                    }
                    else
                    {
                        // Process the command and return any result.

                        messg(ibuf, obuf);
                        sz = strlen(obuf);

                        if (sz > 0) ::send(*i, obuf, sz, 0);
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------

void app::host::init_server(app::node node)
{
    // If we have a server assignment then we must connect to it.

    if (app::node server = find(node, "server"))
    {
        socklen_t  addresslen = sizeof (sockaddr_t);
        sockaddr_t address;

        const char *addr = get_attr_s(server, "addr", DEFAULT_HOST);
        int         port = get_attr_d(server, "port", DEFAULT_PORT);

        // Look up the given host name.

        address.sin_family      = AF_INET;
        address.sin_port        = htons (port);
        address.sin_addr.s_addr = lookup(addr);

        // Create a socket and connect.

        if ((server_sd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
            throw app::sock_error(addr);

        while (connect(server_sd, (struct sockaddr *) &address, addresslen) <0)
            if (sock_errno == ECONNREFUSED)
            {
                std::cerr << "Waiting for " << addr << std::endl;
                usleep(250000);
            }
            else throw app::sock_error(addr);

        nodelay(server_sd);
    }
}

void app::host::fini_server()
{
    if (server_sd != INVALID_SOCKET)
        ::close(server_sd);

    server_sd = INVALID_SOCKET;
}

//-----------------------------------------------------------------------------

void app::host::init_client(app::node node)
{
    app::node curr;

    // Launch all client processes.

    for (curr = find(node,       "client"); curr;
         curr = next(node, curr, "client"))
    {
        fork_client(get_attr_s(curr, "name"),
                    get_attr_s(curr, "addr"));
    }
}

void app::host::fini_client()
{
    while (!client_sd.empty())
    {
        int sd = client_sd.front();
        char c;

        // Wait for EOF (orderly shutdown by the remote).

        while (::recv(sd, &c, 1, 0) > 0)
            ;

        // Close the socket.

        ::close(sd);

        client_sd.pop_front();
    }
}

void app::host::fini_script()
{
    while (!script_sd.empty())
    {
        int sd = script_sd.front();
        char c;

        // Wait for EOF (orderly shutdown by the remote).

        while (::recv(sd, &c, 1, 0) > 0)
            ;

        // Close the socket.

        ::close(sd);

        script_sd.pop_front();
    }
}

//-----------------------------------------------------------------------------

app::host::host(std::string filename, std::string tag) :
    server_sd(INVALID_SOCKET),
    client_cd(INVALID_SOCKET),
    script_cd(INVALID_SOCKET),
    bench(::conf->get_i("bench")),
    movie(::conf->get_i("movie")),
    frame(0),
    calibrate_state(false),
    calibrate_index(0),
    file(filename.c_str())
{
    // Set some reasonable defaults.

    window[0] = 0;
    window[1] = 0;
    window[2] = DEFAULT_PIXEL_WIDTH;
    window[3] = DEFAULT_PIXEL_HEIGHT;

    buffer[0] = DEFAULT_PIXEL_WIDTH;
    buffer[1] = DEFAULT_PIXEL_HEIGHT;

    window_full  = 0;
    window_frame = 1;

    // Read host.xml and configure using tag match.

    app::node root;
    app::node node;
    app::node curr;

    if ((root = find(file.get_head(), "host")))
    {
        // Extract global off-screen buffer configuration.

        if (app::node buf = find(root, "buffer"))
        {
            buffer[0] = get_attr_d(buf, "w", DEFAULT_PIXEL_WIDTH);
            buffer[1] = get_attr_d(buf, "h", DEFAULT_PIXEL_HEIGHT);
        }

        // Locate the configuration for this node.

        if ((node = find(root, "node", "name", tag.c_str())))
        {
            // Extract the on-screen window configuration.

            if (app::node win = find(node, "window"))
            {
                window[0] = get_attr_d(win, "x", 0);
                window[1] = get_attr_d(win, "y", 0);
                window[2] = get_attr_d(win, "w", DEFAULT_PIXEL_WIDTH);
                window[3] = get_attr_d(win, "h", DEFAULT_PIXEL_HEIGHT);

                window_full  = get_attr_d(win, "full",  0);
                window_frame = get_attr_d(win, "frame", 1);
            }

            // Extract local off-screen buffer configuration.

            if (app::node buf = find(node, "buffer"))
            {
                buffer[0] = get_attr_d(buf, "w", DEFAULT_PIXEL_WIDTH);
                buffer[1] = get_attr_d(buf, "h", DEFAULT_PIXEL_HEIGHT);
            }

            // Create a tile object for each configured tile.

            for (curr = find(node,       "tile"); curr;
                 curr = next(node, curr, "tile"))
                tiles.push_back(new tile(curr));

            // Start the network syncronization.

            init_listen(node);
            init_server(node);
            init_client(node);
        }
    }

    // Create a view object for each configured view.

    for (curr = find(root,       "view"); curr;
         curr = next(root, curr, "view"))
        views.push_back(new view(curr, buffer));

    // If no views or tiles were configured, instance defaults.

    if (views.empty()) views.push_back(new view(0, buffer));
    if (tiles.empty()) tiles.push_back(new tile(0));

    // Start the timer.

    tock = SDL_GetTicks();
}

app::host::~host()
{
    std::vector<view *>::iterator i;

    for (i = views.begin(); i != views.end(); ++i)
        delete (*i);
    
    fini_script();
    fini_client();
    fini_server();
    fini_listen();
}

//-----------------------------------------------------------------------------

void app::host::root_loop()
{
    std::vector<tile *>::iterator i;

    double p[3];
    double q[4];

    // TODO: decide on a device numbering for mouse/trackd/joystick

    while (1)
    {
        SDL_Event e;

        // While there are available SDL events, dispatch event handlers.

        while (SDL_PollEvent(&e))
            switch (e.type)
            {
            case SDL_MOUSEMOTION:
                for (i = tiles.begin(); i != tiles.end(); ++i)
                    if ((*i)->pick(p, q, e.motion.x, e.motion.y))
                        point(0, p, q);
                break;

            case SDL_MOUSEBUTTONDOWN:
                click(0, e.button.button, SDL_GetModState(), true);
                break;

            case SDL_MOUSEBUTTONUP:
                click(0, e.button.button, SDL_GetModState(), false);
                break;

            case SDL_KEYDOWN:
                keybd(e.key.keysym.unicode,
                      e.key.keysym.sym,
                      e.key.keysym.mod, true);
                break;

            case SDL_KEYUP:
                keybd(e.key.keysym.unicode,
                      e.key.keysym.sym,
                      e.key.keysym.mod, false);
                break;

            case SDL_JOYAXISMOTION:
                value(e.jaxis.which,
                      e.jaxis.axis,
                      e.jaxis.value / 32768.0);
                break;

            case SDL_JOYBUTTONDOWN:
                click(e.jbutton.which,
                      e.jbutton.button, 0, true);
                break;

            case SDL_JOYBUTTONUP:
                click(e.jbutton.which,
                      e.jbutton.button, 0, false);
                break;

            case SDL_QUIT:
                close();
                return;
            }

        // Check for script input events.

        poll_script();

        // Dispatch tracker events.

        if (tracker_status())
        {
            double p[3];
            double q[3];
            double v;
            bool   b;

            if (tracker_button(0, b)) click(0, 0, 0, b);
            if (tracker_button(1, b)) click(0, 1, 0, b);
            if (tracker_button(2, b)) click(0, 2, 0, b);

            if (tracker_values(0, v)) value(0, 0, v);
            if (tracker_values(1, v)) value(0, 1, v);

            if (tracker_sensor(0, p, q)) point(0, p, q);
            if (tracker_sensor(1, p, q)) point(1, p, q);
        }

        if (bench == 0)  // Advance by JIFFYs until sim clock meets wall clock.
        {
            int tick = SDL_GetTicks();

            while (tick - tock >= JIFFY)
            {
                tock += JIFFY;
                timer(JIFFY);
            }
        }
        if (bench == 1) // Advance by exactly one JIFFY.
        {
            timer(JIFFY);
        }
        if (bench == 2) // Advance sim clock to wall clock in exactly on update.
        {
            int tick = SDL_GetTicks();

            timer(tick - tock);

            tock = tick;
        }

        // Call the paint handler.

        paint();
        front();

        // Count frames and record a movie, if requested.
        
        frame++;

        if (movie && (frame % movie) == 0)
        {
            char buf[256];

            sprintf(buf, "frame%05d.png", frame / movie);

            ::prog->snap(std::string(buf),
                         ::host->get_window_w(),
                         ::host->get_window_h());
        }
    }
}

void app::host::node_loop()
{
    while (1)
    {
        message M(0);

        M.recv(server_sd);

        switch (M.type())
        {
        case E_POINT:
        {
            double p[3];
            double q[4];

            int i = M.get_byte();
            p[0]  = M.get_real();
            p[1]  = M.get_real();
            p[2]  = M.get_real();
            q[0]  = M.get_real();
            q[1]  = M.get_real();
            q[2]  = M.get_real();
            q[3]  = M.get_real();
            point(i, p, q);
            break;
        }
        case E_CLICK:
        {
            int  i = M.get_byte();
            int  b = M.get_byte();
            int  m = M.get_byte();
            bool d = M.get_bool();
            click(i, b, m, d);
            break;
        }
        case E_KEYBD:
        {
            int  c = M.get_word();
            int  k = M.get_word();
            int  m = M.get_word();
            bool d = M.get_bool();
            keybd(c, k, m, d);
            break;
        }
        case E_VALUE:
        {
            int    i = M.get_byte();
            int    a = M.get_byte();
            double p = M.get_real();
            value(i, a, p);
            break;
        }
        case E_MESSG:
        {
            messg(M.get_text(), NULL);
            break;
        }
        case E_TIMER:
        {
            timer(M.get_word());
            break;
        }
        case E_PAINT:
        {
            paint();
            break;
        }
        case E_FRONT:
        {
            front();
            break;
        }
        case E_CLOSE:
        {
            close();
            return;
        }
        default:
            break;
        }
    }
}

void app::host::loop()
{
    // Handle all events.

    if (root())
        root_loop();
    else
        node_loop();
}

//-----------------------------------------------------------------------------

void app::host::draw()
{
    std::vector<frustum *> frusta;

    // Acculumate a list of frusta and preprocess the app.

    for (tile_i i = tiles.begin(); i != tiles.end(); ++i)
        (*i)->prep(views, frusta);

    ::prog->prep(frusta);

    // Render all tiles.
    
//  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int c = 0;

    for (tile_i i = tiles.begin(); i != tiles.end(); ++i)
        (*i)->draw(views, c, calibrate_state, calibrate_index);

    // If doing network sync, wait until the rendering has finished.

    if (server_sd != INVALID_SOCKET || !client_sd.empty())
        glFinish();
}

int app::host::get_window_m() const
{
    return ((window_full  ? SDL_FULLSCREEN : 0) |
            (window_frame ? 0 : SDL_NOFRAME));
}

//-----------------------------------------------------------------------------

void app::host::send(message& M)
{
    // Send a message to all clients.

    for (SOCKET_i i = client_sd.begin(); i != client_sd.end(); ++i)
        M.send(*i);
}

void app::host::recv(message& M)
{
    // Receive a message from all clients (should be E_REPLY).

    for (SOCKET_i i = client_sd.begin(); i != client_sd.end(); ++i)
        M.recv(*i);
}

//-----------------------------------------------------------------------------

void app::host::point(int i, const double *p, const double *q)
{
    // Forward the event to all clients.

    if (!client_sd.empty())
    {
        message M(E_POINT);

        M.put_byte(i);
        M.put_real(p[0]);
        M.put_real(p[1]);
        M.put_real(p[2]);
        M.put_real(q[0]);
        M.put_real(q[1]);
        M.put_real(q[2]);
        M.put_real(q[3]);

        send(M);
    }

    // Let the application have the click event.

    ::prog->point(i, p, q);
}

void app::host::click(int i, int b, int m, bool d)
{
    // Forward the event to all clients.

    if (!client_sd.empty())
    {
        message M(E_CLICK);

        M.put_byte(i);
        M.put_byte(b);
        M.put_byte(m);
        M.put_bool(d);

        send(M);
    }

    // Calibrating a tile?

    if (calibrate_state)
    {
        if (tile_input_click(i, b, m, d))
            return;
    }

    // Let the application have the click event.

    else ::prog->click(i, b, m, d);
}

void app::host::keybd(int c, int k, int m, bool d)
{
    // Forward the event to all clients.

    if (!client_sd.empty())
    {
        message M(E_KEYBD);

        M.put_word(c);
        M.put_word(k);
        M.put_word(m);
        M.put_bool(d);

        send(M);
    }

    if (d && (m & KMOD_CTRL))
    {
        // Cycling display mode?

        if (k == SDLK_RETURN)
        {
            for (tile_i t = tiles.begin(); t != tiles.end(); ++t)
                (*t)->cycle();
            return;
        }

        // Toggling calibration mode?

        else if (k == SDLK_TAB)
        {
            calibrate_state = !calibrate_state;
            printf("state %d\n", calibrate_state);
            return;
        }

        // Selecting calibration tile?

        else if (k == SDLK_SPACE)
        {
            calibrate_index++;
            printf("index %d\n", calibrate_index);
            return;
        }
        else if (k == SDLK_BACKSPACE)
        {
            calibrate_index--;
            printf("index %d\n", calibrate_index);
            return;
        }

        // Calibrating a tile?

        else if (calibrate_state)
        {
            tile_input_keybd(c, k, m, d);
            return;
        }
    }

    // If none of the above, let the application have the keybd event.

    ::prog->keybd(c, k, m, d);
}

void app::host::value(int i, int a, double v)
{
    // Forward the event to all clients.

    if (!client_sd.empty())
    {
        message M(E_VALUE);

        M.put_byte(i);
        M.put_byte(a);
        M.put_real(v);

        send(M);
    }

    // Let the application handle it.

    ::prog->value(i, a, v);
}

void app::host::messg(const char *ibuf, char *obuf)
{
    // Forward the event to all clients.

    if (!client_sd.empty())
    {
        message M(E_MESSG);

        M.put_text(ibuf);

        send(M);
    }

    // Let the application handle it.

    ::prog->messg(ibuf, obuf);
}

void app::host::timer(int t)
{
    // Forward the event to all clients.

    if (!client_sd.empty())
    {
        message M(E_TIMER);

        M.put_word(t);

        send(M);
    }

    // Let the application handle it.

    ::prog->timer(t);

    // Check for connecting clients.

    poll_listen();
}

void app::host::paint()
{
    // Forward the event to all clients, draw, and wait for sync.

    if (!client_sd.empty())
    {
        message M(E_PAINT);

        send(M);
        draw( );
        recv(M);
    }
    else draw();

    // Send sync to the server.

    if (server_sd != INVALID_SOCKET)
    {
        message R(E_REPLY);
        R.send(server_sd);
    }
}

void app::host::front()
{
    // Forward the event to all clients.

    if (!client_sd.empty())
    {
        message M(E_FRONT);

        send(M);
    }

    // Swap the back buffer to the front.

    SDL_GL_SwapBuffers();
    ::perf->step(bench);
}

void app::host::close()
{
    // Forward the event to all clients.  Wait for sync.

    if (!client_sd.empty())
    {
        message M(E_CLOSE);

        send(M);
        recv(M);
    }

    // Send sync to the server.

    if (server_sd != INVALID_SOCKET)
    {
        message R(E_REPLY);
        R.send(server_sd);
    }
}

//-----------------------------------------------------------------------------

void app::host::set_head(const double *p,
                         const double *q)
{
    std::vector<view *>::iterator i;

    for (i = views.begin(); i != views.end(); ++i)
        (*i)->set_head(p, q);
}

//-----------------------------------------------------------------------------
#ifdef SNIP
void app::host::gui_pick(int& x, int& y, const double *p,
                                         const double *q) const
{
    // Compute the point (x, y) at which the vector V from P hits the GUI.

    double q[3];
    double w[3];

    // TODO: convert this to take a quaternion

    mult_mat_vec3(q, gui_I, p);
    mult_xps_vec3(w, gui_I, v);

    normalize(w);

    x = int(nearestint(q[0] - q[2] * w[0] / w[2]));
    y = int(nearestint(q[1] - q[2] * w[1] / w[2]));
}

void app::host::gui_size(int& w, int& h) const
{
    // Return the configured resolution of the GUI.

    w = gui_w;
    h = gui_h;
}

void app::host::gui_view() const
{
    // Apply the transform taking GUI coordinates to eye coordinates.

    glMultMatrixd(gui_M);
}
#endif
//-----------------------------------------------------------------------------

bool app::host::tile_input_point(int i, const double *p, const double *q)
{
    for (tile_i t = tiles.begin(); t != tiles.end(); ++t)
        if ((*t)->is_index(calibrate_index) && (*t)->input_point(i, p, q))
            return true;

    return false;
}

bool app::host::tile_input_click(int i, int b, int m, bool d)
{
    for (tile_i t = tiles.begin(); t != tiles.end(); ++t)
        if ((*t)->is_index(calibrate_index) && (*t)->input_click(i, b, m, d))
            return true;

    return false;
}

bool app::host::tile_input_keybd(int c, int k, int m, bool d)
{
    for (tile_i t = tiles.begin(); t != tiles.end(); ++t)
        if ((*t)->is_index(calibrate_index) && (*t)->input_keybd(c, k, m, d))
            return true;

    return false;
}

//-----------------------------------------------------------------------------
