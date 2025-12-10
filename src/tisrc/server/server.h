#ifndef TISRC_SERVER_SERVER_H
#define TISRC_SERVER_SERVER_H

int parseargs(int argc, char** argv);
int bootstrap(void);
void unstrap(void);
void loop(void);

#endif
