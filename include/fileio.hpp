#ifndef FILEIO_HPP
#define FILEIO_HPP

#include "database.hpp"
#include <string>

void saveDatabase(Database& db, string directory);
void loadDatabase(Database& db, string directory);

#endif
