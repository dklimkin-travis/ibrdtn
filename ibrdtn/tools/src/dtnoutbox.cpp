/*
 * dtnoutbox.cpp
 *
 * Copyright (C) 2011 IBR, TU Braunschweig
 *
 * Written-by: Johannes Morgenroth <morgenroth@ibr.cs.tu-bs.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "config.h"
#include "ibrdtn/api/Client.h"
#include "ibrcommon/net/socket.h"
#include "ibrcommon/thread/Mutex.h"
#include "ibrcommon/thread/MutexLock.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrcommon/data/BLOB.h"
#include "ibrcommon/data/File.h"
#include "ibrcommon/appstreambuf.h"

#include "TarUtils.h"


#define HAVE_LIBTFFS 1

#ifdef HAVE_LIBTFFS
extern "C"
{
#include "tffs/tffs.h"
#include "FATFile.h"
}
#endif

#include "ObservedFile.h"
#include "ObservedFile.cpp" //TODO doof!

#include <stdlib.h>
#include <iostream>
#include <map>
#include <vector>
#include <csignal>
#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#endif


using namespace ibrcommon;

void print_help()
{
	cout << "-- dtnoutbox (IBR-DTN) --" << endl;
	cout << "Syntax: dtnoutbox [options] <name> <outbox> <destination>" << endl;
	cout << " <name>             the application name" << endl;
#ifdef HAVE_LIBTFFS
	cout << " <outbox>           location of outgoing files, only vfat-images (*.img)" << endl;
#else
	cout << " <outbox>           directory of outgoing files" << endl;
#endif
	cout << " <destination>      the destination EID for all outgoing files" << endl << endl;
	cout << "* optional parameters *" << endl;
	cout << " -h|--help          display this text" << endl;
	cout << " -w|--workdir <dir> temporary work directory" << endl;
	cout << " -i <interval>      interval in milliseconds, in which <outbox> is scanned for new/changed files. default: 5000" << endl;
	cout << " -r <number>        number of rounds of intervals, after which a unchanged file is considered as written. default: 3" << endl;
#ifdef HAVE_LIBTFFS
	cout << " -p <path>          path of outbox within vfat image. default: /" << endl;
#endif
	cout << endl;
	cout << " --badclock         assumes a bad clock on the system, the only indicator to send a file is its size" << endl;
	cout << " --consider-swp     do not ignore these files: *~* and *.swp" << endl;
	cout << " --consider-invis   do not ignores these files: .*" << endl;
	cout << " --no-keep          do no t keep files in outbox" << endl;
	cout << " --quiet		     only print error messages" << endl;

}

map<string,string> readconfiguration( int argc, char** argv )
{
	// print help if not enough parameters are set
	if (argc < 4)
	{
		print_help();
		exit(0);
	}

	map<string,string> ret;
	ret["name"] = argv[1];
	ret["outbox"] = argv[2];
	ret["destination"] = argv[3];
	//default values:
	ret["interval"] = "5000";
	ret["rounds"] = "3";
	ret["path"] = "/";

	for (int i = 4; i < argc; ++i)
	{
		string arg = argv[i];

		// print help if requested
		if (arg == "-h" || arg == "--help")
		{
			print_help();
			exit(0);
		}

		if (arg == "-w" || arg == "--workdir")
			ret["workdir"] = argv[++i];

		if (arg == "-i")
			ret["interval"] = argv[++i];

		if (arg == "-r")
			ret["rounds"] = argv[++i];

		if (arg == "--badclock")
			ret["badclock"] = "1";

		if (arg == "--consider-swp")
			ret["consider_swp"] = "1";

		if (arg == "--consider-invis")
			ret["consider_invis"] = "1";

		if ( arg == "--no-keep")
			ret["no-keep"] = "1";

		if ( arg == "--quiet")
			ret["quiet"] = "1";

		if (arg == "-p")
			ret["path"] = argv[++i];

	}

	return ret;
}

// set this variable to false to stop the app
bool _running = true;

// global connection
ibrcommon::socketstream *_conn = NULL;

void term( int signal )
{
	if (signal >= 1)
	{
		_running = false;
		if (_conn != NULL) _conn->close();
	}
}

static bool isSwap(string filename)
{
	bool tilde = filename.find("~",0) != filename.npos;
	bool swp   = filename.find(".swp",filename.length()-4) != filename.npos;

	return (tilde || swp);
}

static bool isInvis(string filename)
{
	return filename.at(0) == '.';
}

/*
 * main application method
 */
int main( int argc, char** argv )
{
	// catch process signals
	signal(SIGINT, term);
	signal(SIGTERM, term);

	// read the configuration
	map<string,string> conf = readconfiguration(argc, argv);
	size_t _conf_interval = atoi(conf["interval"].c_str());
	size_t _conf_rounds = atoi(conf["rounds"].c_str());

	//check keep parameter
	bool _conf_keep = true;
	if (conf.find("no-keep") != conf.end())
	{
		_conf_keep = false;
	}

	//check badclock parameter
	bool _conf_badclock = false;
	if (conf.find("badclock") != conf.end())
	{
		_conf_badclock= true;
	}

	//check consider-swp parameter
	bool _conf_consider_swp = false;
	if (conf.find("consider_swp") != conf.end())
	{
		_conf_consider_swp = true;
	}

	//check consider-invis parameter
	bool _conf_consider_invis = false;
	if (conf.find("consider_invis") != conf.end())
	{
		_conf_consider_invis = true;
	}

	//check consider-invis parameter
	bool _conf_quiet = false;
	if (conf.find("quiet") != conf.end())
	{
		_conf_quiet = true;
	}

	// init working directory
	if (conf.find("workdir") != conf.end())
	{
		ibrcommon::File blob_path(conf["workdir"]);

		if (blob_path.exists())
		{
			ibrcommon::BLOB::changeProvider(new ibrcommon::FileBLOBProvider(blob_path), true);
		}
	}


	// backoff for reconnect
	unsigned int backoff = 2;

	// check outbox for files
#ifdef HAVE_LIBTFFS
		File outbox_img(conf["outbox"]);
		FATFile::setImgPath(conf["outbox"]);
		FATFile outbox(conf["path"]);
		list<FATFile> avail_files, new_files, old_files, deleted_files;
		list<FATFile>::iterator iter;
		list<ObservedFile<FATFile> > observed_files;
		list<ObservedFile<FATFile>* > files_to_send;
		list<ObservedFile<FATFile> >::iterator of_iter;
		list<ObservedFile<FATFile>* >::iterator of_ptr_iter;
		// TODO create vfat image, if nessecary

		if(outbox_img.getPath().substr(outbox_img.getPath().length()-4) != ".img")
		{
			cout << "ERROR: this is not an img file" << endl;
			return -1;
		}

		if(!outbox_img.exists())
		{
			cout << "ERROR: img file not found" << endl;
			return -1;
		}

#else
		File outbox(conf["outbox"]);
		list<File> avail_files, new_files, old_files, deleted_files;
		list<File>::iterator iter;
		list<ObservedFile<File> > observed_files;
		list<ObservedFile<File>* > files_to_send;
		list<ObservedFile<File> >::iterator of_iter;
		list<ObservedFile<File>* >::iterator of_ptr_iter;

		if(!outbox.isDirectory())
		{
			cout << "ERROR: this is not a directory" << endl;
			return -1;
		}
		if(!outbox.exists())
			File::createDirectory(outbox);
#endif

	// loop, if no stop if requested
	while (_running)
	{
		try
		{
			// Create a stream to the server using TCP.
			ibrcommon::vaddress addr("localhost", 4550);
			ibrcommon::socketstream conn(new ibrcommon::tcpsocket(addr));

			// set the connection globally
			_conn = &conn;

			// Initiate a client for synchronous receiving
			dtn::api::Client client(conf["name"], conn, dtn::api::Client::MODE_SENDONLY);

			// Connect to the server. Actually, this function initiate the
			// stream protocol by starting the thread and sending the contact header.
			client.connect();

			// reset backoff if connected
			backoff = 2;

			// check the connection
			while (_running)
			{

				deleted_files.clear();
				new_files.clear();
				old_files.clear();

				//store old files
				old_files = avail_files;
				avail_files.clear();

				//get all files
				outbox.getFiles(avail_files);

				//determine deleted files
				set_difference(old_files.begin(),old_files.end(),avail_files.begin(),avail_files.end(),std::back_inserter(deleted_files));
				//determine new files
				set_difference(avail_files.begin(),avail_files.end(),old_files.begin(),old_files.end(),std::back_inserter(new_files));

				//remove deleted files from observation
				for (iter = deleted_files.begin(); iter != deleted_files.end(); ++iter)
				{
					ObservedFile<typeof(*iter)> toFind((*iter).getPath());
					of_iter = std::find(observed_files.begin(),observed_files.end(),toFind);
					observed_files.erase(of_iter);

				}

				//add new files to observation
				for (iter = new_files.begin(); iter != new_files.end(); ++iter)
				{
					// skip system files ("." and "..")
					if ((*iter).isSystem()) continue;

					//skip invisible and swap-files, if wanted
					if (!_conf_consider_swp && isSwap((*iter).getBasename())) continue;
					if (!_conf_consider_invis && isInvis((*iter).getBasename())) continue;

					observed_files.push_back(ObservedFile<typeof(*iter)>((*iter).getPath()));
				}


				if (observed_files.size() == 0)
				{
					if (!_conf_quiet)
						cout << "0 files to send: directory empty" << endl;

					// wait some seconds
					ibrcommon::Thread::sleep(_conf_interval);

					continue;
				}

				//find files to send
				files_to_send.clear();
				for (of_iter = observed_files.begin(); of_iter != observed_files.end(); ++of_iter)
				{
					ObservedFile<typeof(*iter)> &of = (*of_iter);
					of.addSize(); //measure current size

					size_t latest_timestamp = of.getLastTimestamp();
					//two indicators to send:
					bool send_time = (of.getLastSent() < latest_timestamp) || _conf_badclock;
					bool send_size = of.lastSizesEqual(_conf_rounds);
					if (send_time && send_size)
					{
						files_to_send.push_back(&of);
					}
				}

				//mark files as send and create list for tar
				size_t counter = 0;
				stringstream files_to_send_ss;
				for(of_ptr_iter = files_to_send.begin(); of_ptr_iter != files_to_send.end(); of_ptr_iter++)
				{
							(*of_ptr_iter)->send();
							files_to_send_ss << (*of_ptr_iter)->getBasename() << " ";
							counter++;
				}

				if (!counter)
				{
					if(!_conf_quiet)
						cout << "0 files to send: requirements not fulfilled" << endl;
				}
				else
				{
					if(!_conf_quiet)
					{
						string s = " ";
						size_t size = files_to_send.size();
						if(size > 1) s = "s";

						cout << files_to_send.size() << " file" << s << " to send: " << files_to_send_ss.str() << endl;
					}

					// create a blob
					ibrcommon::BLOB::Reference blob = ibrcommon::BLOB::create();

//use libarchive...
#ifdef HAVE_LIBARCHIVE
	//if libarchive is available, check if libtffs can be used
	#ifdef HAVE_LIBTFFS
					TarUtils::set_img_path(conf["outbox"]);
	#endif
					TarUtils::write_tar_archive(&blob, files_to_send);

					//delete files, if wanted
					if (!_conf_keep)
					{
						iter = avail_files.begin();
						while (iter != avail_files.end())
						{
							(*iter++).remove(true);
						}
					}
//or commandline fallback
#else
					// "--remove-files" deletes files after adding
					//depending on configuration, this option is passed to tar or not
					std::string remove_string = " --remove-files";
					if(_conf_keep)
						remove_string = "";
					stringstream cmd;
					cmd << "tar" << remove_string << " -cO -C " << outbox.getPath() << " " << files_to_send_ss.str();

					// make a tar archive
					appstreambuf app(cmd.str(), appstreambuf::MODE_READ);
					istream stream(&app);

					// stream the content of "tar" to the payload block
					(*blob.iostream()) << stream.rdbuf();
#endif
					// create a new bundle
					dtn::data::EID destination = EID(conf["destination"]);

					// create a new bundle
					dtn::data::Bundle b;

					// set destination
					b.destination = destination;

					// add payload block using the blob
					b.push_back(blob);

					// send the bundle
					client << b;
					client.flush();
				}
				if (_running)
				{
					// wait some seconds
					ibrcommon::Thread::sleep(_conf_interval);
				}
			}

			// close the client connection
			client.close();

			// close the connection
			conn.close();

			// set the global connection to NULL
			_conn = NULL;
		} catch (const ibrcommon::socket_exception&)
		{
			// set the global connection to NULL
			_conn = NULL;

			if (_running)
			{
				cout << "Connection to bundle daemon failed. Retry in " << backoff << " seconds." << endl;
				ibrcommon::Thread::sleep(backoff * 1000);

				// if backoff < 10 minutes
				if (backoff < 600)
				{
					// set a new backoff
					backoff = backoff * 2;
				}
			}
		} catch (const ibrcommon::IOException&)
		{
			// set the global connection to NULL
			_conn = NULL;

			if (_running)
			{
				cout << "Connection to bundle daemon failed. Retry in " << backoff << " seconds." << endl;
				ibrcommon::Thread::sleep(backoff * 1000);

				// if backoff < 10 minutes
				if (backoff < 600)
				{
					// set a new backoff
					backoff = backoff * 2;
				}
			}
		} catch (const std::exception&)
		{
			// set the global connection to NULL
			_conn = NULL;
		}
	}

	return (EXIT_SUCCESS);
}
