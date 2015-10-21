#ifndef _MESSAGE_H
#define _MESSAGE_H 1

#include <unordered_map>
#include <string>
#include <vector>

/**
 * Forward declaration of class.
 */
class CMessagePart;


/**
 * This is the C++ object which represents an email message.
 *
 * The lua binding is modeled after this class structure too.
 *
 */
class CMessage
{
  public:
    /**
     * Constructor.
     */
    CMessage (const std::string name);

    /**
     * Destructor.
     */
         ~CMessage ();

    /**
     * Get the path of this message.
     */
          std::string path ();


    /**
     * Update the path to the message.
     */
    void path (std::string new_path);


    /**
     * Get the value of the given header.
     */
         std::string header (std::string name);

    /**
     * Get all headers, and their values.
     */
         std::unordered_map < std::string, std::string > headers ();

    /**
     * Retrieve the current flags for this message.
     */
         std::string get_flags ();

    /**
     * Set the flags for this message.
     */
    void set_flags (std::string new_flags);

    /**
     * Add a flag to a message.
     */
    bool add_flag (char c);

    /**
     * Does this message possess the given flag?
     */
    bool has_flag (char c);

    /**
     * Remove a flag from a message.
     */
    bool remove_flag (char c);

    /**
     * Is this message new?
     */
    bool is_new ();


    /**
     * Get message-parts
     */
         std::vector < CMessagePart * >get_parts ();

  private:

    /**
     * The path on-disk to the message.
     */
         std::string m_path;

     /**
      * Cached MIME-parts to this message.
      */
         std::vector < CMessagePart * >m_parts;

};



#endif /* _MESSAGE_H  */
