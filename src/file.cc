
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>


#include "file.h"
#include "maildir.h"


#ifndef FILE_READ_BUFFER
#define FILE_READ_BUFFER 16384
#endif



/**
 * Test if a file exists.
 */
bool
CFile::exists (std::string path)
{
    struct stat sb;

    if ((stat (path.c_str (), &sb) == 0))
	return true;
    else
	return false;
}


/**
 * Is the given file executable?
 */
bool
CFile::executable (std::string path)
{
    struct stat sb;

    /**
     * Fail to stat?  Then not executable.
     */
    if (stat (path.c_str (), &sb) < 0)
	return false;

    /**
     * If directory then not executable.
     */
    if (S_ISDIR (sb.st_mode))
	return false;

    /**
     * Otherwise check the mode-bits
     */
    if ((sb.st_mode & S_IEXEC) != 0)
	return true;
    return false;
}


/**
 * Is the given path a directory?
 */
bool
CFile::is_directory (std::string path)
{
    struct stat sb;

    if (stat (path.c_str (), &sb) < 0)
	return false;

    return (S_ISDIR (sb.st_mode));
}



/**
 * Get the files in the given directory.
 *
 * NOTE: Directories are excluded.
 */
std::vector < std::string > CFile::files_in_directory (std::string path)
{
    std::vector < std::string > results;

    assert (CFile::is_directory (path) == true);

    dirent *
	de;
    DIR *
	dp;

    dp = opendir (path.c_str ());
    if (dp)
    {
	std::string base = path;
	base += "/";

	while (true)
	{
	    de = readdir (dp);
	    if (de == NULL)
		break;

	    /**
             * Ignore . + ..
             */
	    if ((strcmp (de->d_name, ".") != 0) &&
		(strcmp (de->d_name, "..") != 0))
	    {
		/**
                 * Build up the complete file.
                 */
		std::string file = base + de->d_name;

		/**
                 * Return the results.
                 */
		if (!CFile::is_directory (file))
		    results.push_back (file);
	    }
	}
	closedir (dp);
    }

    std::sort (results.begin (), results.end ());
    return (results);
}


/**
 * Remove a file.
 */
bool
CFile::delete_file (std::string path)
{
    /**
     * Ensure we're not removing a directory.
     */
    assert (!CFile::is_directory (path));

    /**
     * Unlink.
     */
    bool result = unlink (path.c_str ());

    /**
     * Test that the file was removed.
     */
    assert (!CFile::exists (path));

    return (result);
}


/**
 * Get the basename of a file.
 */
std::string CFile::basename (std::string path)
{
    size_t
	offset = path.find_last_of ("/");

    if (offset != std::string::npos)
	return (path.substr (offset + 1));
    else
	return (path);
}


/**
 * Copy a file.
 */
void
CFile::copy (std::string src, std::string dst)
{

    std::ifstream isrc (src, std::ios::binary);
    std::ofstream odst (dst, std::ios::binary);

    std::istreambuf_iterator < char >begin_source (isrc);
    std::istreambuf_iterator < char >end_source;
    std::ostreambuf_iterator < char >begin_dest (odst);

    std::copy (begin_source, end_source, begin_dest);

    isrc.close ();
    odst.close ();

    assert (CFile::exists (dst));
}



/**
 * Move a file.
 */
bool
CFile::move (std::string src, std::string dst)
{
    int ret = rename (src.c_str (), dst.c_str ());

    assert (CFile::exists (dst));
    assert (!CFile::exists (src));

    return (ret == 0);
}


/**
 * Send the contents of a file to the given command, via popen.
 */
bool
CFile::file_to_pipe (std::string src, std::string cmd)
{

    char buf[FILE_READ_BUFFER] = { '\0' };
    ssize_t nread;

    FILE *file = fopen (src.c_str (), "r");
    if (file == NULL)
	return false;

    FILE *pipe = popen (cmd.c_str (), "w");
    if (pipe == NULL)
	return false;

    /**
     * While we read from the file send to pipe.
     */
    while ((nread = fread (buf, sizeof (char), sizeof buf - 1, file)) > 0)
    {
	char *out_ptr = buf;
	ssize_t nwritten;

	do
	{
	    nwritten = fwrite (out_ptr, sizeof (char), nread, pipe);

	    if (nwritten >= 0)
	    {
		nread -= nwritten;
		out_ptr += nwritten;
	    }
	}
	while (nread > 0);

	memset (buf, '\0', sizeof (buf));
    }

    pclose (pipe);
    fclose (file);

    return true;
}


/**
 * Return a sorted list of maildirs beneath the given prefix.
 */
std::vector < std::string > CFile::get_all_maildirs (std::string prefix)
{
    std::vector < std::string > result;
    dirent *
	de;
    DIR *
	dp;

    prefix = prefix.empty ()? "." : prefix.c_str ();
    dp = opendir (prefix.c_str ());
    if (dp)
    {
	if (CMaildir::is_maildir (prefix))
	    result.push_back (prefix);

	while (true)
	{
	    de = readdir (dp);
	    if (de == NULL)
		break;

	    if ((strcmp (de->d_name, ".") != 0) &&
		(strcmp (de->d_name, "..") != 0) &&
		((de->d_type == DT_UNKNOWN) || (de->d_type == DT_DIR)))
	    {
		std::string subdir_name = std::string (de->d_name);
		std::string subdir_path =
		    std::string (prefix + "/" + subdir_name);
		if (CMaildir::is_maildir (subdir_path))
		    result.push_back (subdir_path);
		else
		{
		    if (subdir_name != "." && subdir_name != "..")
		    {
			DIR *
			    sdp = opendir (subdir_path.c_str ());
			if (sdp)
			{
			    closedir (sdp);
			    std::vector < std::string > sub_maildirs;
			    sub_maildirs =
				CFile::get_all_maildirs (subdir_path);
			  for (std::string sub_maildir:sub_maildirs)
			    {
				result.push_back (sub_maildir);
			    }
			}
		    }
		}
	    }
	}
	closedir (dp);
	std::sort (result.begin (), result.end ());
    }

    return result;
}


/**
 * Allow completion of file/path-names
 */
std::vector < std::string > CFile::complete_filename (std::string path)
{
    std::vector < std::string > result;

    std::string dir = getcwd (NULL, 0);
    std::string file;

    /**
     * Get the directory-name.
     */
    size_t
	offset = path.find_last_of ("/");
    if (offset != std::string::npos)
    {
	dir = path.substr (0, offset);
	file = path.substr (offset + 1);
    }
    else
    {
	file = path;
    }

    /**
     * Ensure we have a trailing "/".
     */
    if ((dir.empty ()) || (!dir.empty () && (dir.rbegin ()[0] != '/')))
	dir += "/";


    /**
     * Open the directory.
     */
    DIR *
	dp = opendir (dir.c_str ());
    if (dp != NULL)
    {
	while (true)
	{
	    dirent *
		de = readdir (dp);
	    if (de == NULL)
		break;

	    /**
             * Skip dots..
             */
	    if ((strcmp (de->d_name, ".") != 0) &&
		(strcmp (de->d_name, "..") != 0) &&
		(strncasecmp (file.c_str (), de->d_name, file.size ()) == 0))
	    {

		/**
                 * If completing a directory add the trailing "/"
                 * automatically.
                 */
		std::string option = dir + de->d_name;
		if (CFile::is_directory (option))
		    option += "/";

		result.push_back (option);
	    }
	}
	closedir (dp);
    }


    return (result);
}
