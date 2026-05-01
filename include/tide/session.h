#ifndef TIDE_SESSION_H
#define TIDE_SESSION_H

#include "tide/status.h"
#include "tide/workspace.h"

TideStatus tide_session_save_workspace(const TideWorkspace *workspace, const char *path);
TideStatus tide_session_load_workspace(TideWorkspace *workspace, const char *path);

#endif
