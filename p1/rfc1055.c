#include <unistd.h>

/* these two may be same socket */
int send_sock_fd;    // must be established by calling pgm
int recv_sock_fd;    // must be established by calling pgm

void send_char(unsigned char c)
{
    write(send_sock_fd,&c,1);
}

unsigned char recv_char()
{
    unsigned char c;

    read(recv_sock_fd,&c,1);
    return c;
}

/****
The following C language functions send and receive SLIP packets.
They depend on two functions, send_char() and recv_char(), which send
and receive a single character over the serial line.
****/

/* SLIP special character codes */
#define END             0300    /* indicates end of packet */
#define ESC             0333    /* indicates byte stuffing */
#define ESC_END         0334    /* ESC ESC_END means END data byte */
#define ESC_ESC         0335    /* ESC ESC_ESC means ESC data byte */

/* SEND_PACKET: sends a packet of length "len", starting at location "p".  */
void send_packet(unsigned char *p, int len)
{

    /* send an initial END character to flush out any data that may
     * have accumulated in the receiver due to line noise
     */
    send_char(END);

    /* for each byte in the packet, send the appropriate character sequence */
    while (len--)
    {
        switch (*p)
        {
            /* if it's the same code as an END character, we send a
            * special two character code so as not to make the
            * receiver think we sent an END
            */
            case END:
                send_char(ESC);
                send_char(ESC_END);
                break;

            /* if it's the same code as an ESC character,
            * we send a special two character code so as not
            * to make the receiver think we sent an ESC
            */
            case ESC:
                send_char(ESC);
                send_char(ESC_ESC);
                break;

            /* otherwise, we just send the character */
            default:
                send_char(*p);
        }
        p++;
    }

    /* tell the receiver that we're done sending the packet */
    send_char(END);
}

/* RECV_PACKET: receives a packet into the buffer located at "p".
*      If more than len bytes are received, the packet will
*      be truncated.
*      Returns the number of bytes stored in the buffer.
*/
int recv_packet(unsigned char *p, int len)
{
    unsigned char c;
    int received = 0;

    /* Sit in a loop reading bytes until we put together a whole packet.
    *  Make sure not to copy them into the packet if we run out of room.
    */
    while (1)
    {
        /* get a character to process */
        c = recv_char();

        /* handle bytestuffing if necessary */
        switch (c)
        {

            /* if it's an END character then we're done with the packet */
            case END:
            /* a minor optimization: if there is no
            * data in the packet, ignore it. This is
            * meant to avoid bothering IP with all
            * the empty packets generated by the
            * duplicate END characters which are in
            * turn sent to try to detect line noise.
            */
            if (received)
                return received;
            else
                break;

            /* if it's the same code as an ESC character, wait
            * and get another character and then figure out
            * what to store in the packet based on that.
            */
            case ESC:
                c = recv_char();

                /* if "c" is not one of these two, then we
                * have a protocol violation.  The best bet
                * seems to be to leave the byte alone and
                * just stuff it into the packet
                */
                switch(c)
                {
                    case ESC_END:
                        c = END;
                        break;
                    case ESC_ESC:
                        c = ESC;
                        break;
                }

            /* here we fall into the default handler and let
            * it store the character for us
            */
            default:
                if(received < len)
                    p[received++] = c;
        }
    }
}
