/*
 * Configuration reader for IPC Manager
 *
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef __IPCM_CONFIGURATION_H__
#define __IPCM_CONFIGURATION_H__

#include <string>

#include "rina-configuration.h"

namespace rinad {

bool parse_configuration(std::string& file_loc);
DIFTemplate * parse_dif_template(const std::string& file_name,
				 const std::string& template_name);
bool parse_app_to_dif_mappings(const std::string& file_name,
			       std::list< std::pair<std::string, std::string> >& mappings);

}
#endif  /* __IPCM_CONFIGURATION_H__ */
