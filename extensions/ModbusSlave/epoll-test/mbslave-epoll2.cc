#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>


#define EPOLL_ARRAY_SIZE   64


void sprint_buffer(const char *buffer, int size)
{
   int i;
   for (i = 0; i < size; i++)
   {
      if (isprint(buffer[i]))
         printf("%c", buffer[i]);
      else
         printf("\\0x%02X", buffer[i]);
   }
}

int main(int argc, char *argv[])
{
   int sd, efd, clientsd, fd;
   struct sockaddr_in bindaddr, peeraddr;
   socklen_t salen = sizeof(peeraddr);
   int pollsize = 1;
   struct epoll_event ev;
   struct epoll_event epoll_events[EPOLL_ARRAY_SIZE];
   uint32_t events;
   unsigned short port = 20000;
   int i, rval, on = 1;
   ssize_t rc;
   char buffer[1024];


   efd = epoll_create(pollsize);

   if (efd < 0)
   {
      printf("Could not create the epoll fd: %m");
      return 1;
   }

   sd = socket(AF_INET, SOCK_STREAM, 0);
   if (sd < 0)
   {
      printf("Could not create new socket: %m\n");
      return 2;
   }

   printf("New socket created with sd %d\n", sd);

   if (fcntl(sd, F_SETFL, O_NONBLOCK))
   {
      printf("Could not make the socket non-blocking: %m\n");
      close(sd);
      return 3;
   }

   if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
   {
      printf("Could not set socket %d option for reusability: %m\n", sd);
      close(sd);
      return 4;
   }

   bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   bindaddr.sin_family= AF_INET;
   bindaddr.sin_port = htons(port);

   if (bind(sd, (struct sockaddr *) &bindaddr, sizeof(struct sockaddr_in)) < 0)
   {
      printf("Could not bind socket %d to address 'INADDR_ANY' and port %u: %m", sd, port);
      close(sd);
      return 5;
   }
   else
   {
      printf("Bound socket %d to address 'INADDR_ANY' and port %u\n", sd, port);
   }

   if (listen(sd, SOMAXCONN))
   {
      printf("Could not start listening on server socket %d: %m\n", sd);
      goto cleanup;
   }
   else
   {
      printf("Server socket %d started listening to address 'INADDR_ANY' and port %u\n", sd, port);
   }

   ev.events = EPOLLIN;
   ev.data.u64 = 0LL;
   ev.data.fd = sd;

   if (epoll_ctl(efd, EPOLL_CTL_ADD, sd, &ev) < 0)
   {
      printf("Couldn't add server socket %d to epoll set: %m\n", sd);
      goto cleanup;
   }

   for (;;)
   {
      printf("Starting epoll_wait on %d fds\n", pollsize);

      while ((rval = epoll_wait(efd, epoll_events, EPOLL_ARRAY_SIZE, -1)) < 0)
      {
         if ((rval < 0) && (errno != EINTR))
         {
            printf("EPoll on %d fds failed: %m\n", pollsize);
            goto cleanup;
         }
      }

      for (i = 0; i < rval; i++)
      {
         events = epoll_events[i].events;
         fd = epoll_events[i].data.fd;

         if (events & EPOLLERR)
         {
            if (fd == sd)
            {
               printf("EPoll on %d fds failed: %m\n", pollsize);
               goto cleanup;
            } else
            {
               printf("Closing socket with sd %d\n", fd);
               shutdown(fd, SHUT_RDWR);
               close(fd);
               continue;
            }
         }

         if (events & EPOLLHUP)
         {
            if (fd == sd)
            {
               printf("EPoll on %d fds failed: %m\n", pollsize);
               goto cleanup;
            } else
            {
               printf("Closing socket with sd %d\n", fd);
               shutdown(fd, SHUT_RDWR);
               close(fd);
               continue;
            }
         }

         if (events & EPOLLRDHUP)
         {
            if (fd == sd)
            {
               printf("EPoll on %d fds failed: %m\n", pollsize);
               goto cleanup;
            } else
            {
               printf("Closing socket with sd %d\n", fd);
               shutdown(fd, SHUT_RDWR);
               close(fd);
               continue;
            }
         }

         if (events & EPOLLOUT)
         {
            if (fd != sd)
            {
               rc = snprintf(buffer, sizeof(buffer), "Hello socket %d from server socket %d!\n", fd, sd);
               while ((rc = send(fd, buffer, rc, 0)) < 0)
               {
                  if ((fd < 0) && (errno != EINTR))
                  {
                     printf("Send to socket %d failed: %m\n", fd);
                     pollsize--;
                     shutdown(fd, SHUT_RDWR);
                     close(fd);
                     continue;
                  }
               }

               if (rc == 0)
               {
                  printf("Closing socket with sd %d\n", fd);
                  pollsize--;
                  shutdown(fd, SHUT_RDWR);
                  close(fd);
                  continue;
               } else if (rc > 0)
               {
                  printf("Sent '");
                  sprint_buffer(buffer, rc);
                  printf("' to socket with sd %d\n", fd);

                  ev.events = EPOLLIN;
                  ev.data.u64 = 0LL;
                  ev.data.fd = fd;

                  if (epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev) < 0)
                  {
                     printf("Couldn't modify client socket %d in epoll set: %m\n", fd);
                     goto cleanup;
                  }
               }
            }
         }

         if (events & EPOLLIN)
         {
            if (fd == sd)
            {
               while ((clientsd = accept(sd, (struct sockaddr *) &peeraddr, &salen)) < 0)
               {
                  if ((clientsd < 0) && (errno != EINTR))
                  {
                     printf("Accept on socket %d failed: %m\n", sd);
                     goto cleanup;
                  }
               }

               if (inet_ntop(AF_INET, &peeraddr.sin_addr.s_addr, buffer, sizeof(buffer)) != NULL)
               {
                  printf("Accepted connection from %s:%u, assigned new sd %d\n", buffer, ntohs(peeraddr.sin_port), clientsd);
               } else
               {
                  printf("Failed to convert address from binary to text form: %m\n");
               }

               ev.events = EPOLLIN;
               ev.data.u64 = 0LL;
               ev.data.fd = clientsd;

               if (epoll_ctl(efd, EPOLL_CTL_ADD, clientsd, &ev) < 0)
               {
                  printf("Couldn't add client socket %d to epoll set: %m\n", clientsd);
                  goto cleanup;
               }

               pollsize++;
            } else
            {
               while ((rc = recv(fd, buffer, sizeof(buffer), 0)) < 0)
               {
                  if ((fd < 0) && (errno != EINTR))
                  {
                     printf("Receive from socket %d failed: %m\n", fd);
                     pollsize--;
                     shutdown(fd, SHUT_RDWR);
                     close(fd);
                     continue;
                  }
               }

               if (rc == 0)
               {
                  printf("Closing socket with sd %d\n", fd);
                  pollsize--;
                  shutdown(fd, SHUT_RDWR);
                  close(fd);
                  continue;
               } else if (rc > 0)
               {
                  printf("Received '");
                  sprint_buffer(buffer, rc);
                  printf("' from socket with sd %d\n", fd);

                  ev.events = EPOLLIN | EPOLLOUT;
                  ev.data.u64 = 0LL;
                  ev.data.fd = fd;

                  if (epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev) < 0)
                  {
                     printf("Couldn't modify client socket %d in epoll set: %m\n", fd);
                     goto cleanup;
                  }
               }
            }
         }
      }
   }

cleanup:
   shutdown(sd, SHUT_RDWR);
   close(sd);

   return 0;
}