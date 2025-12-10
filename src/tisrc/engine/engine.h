#ifndef TISRC_ENGINE_ENGINE_H
#define TISRC_ENGINE_ENGINE_H

int parseargs(int argc, char** argv);
int bootstrap(void);
void unstrap(void);
void loop(void);

#endif
