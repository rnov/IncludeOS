#include <os>
#include <net/inet4>
#include <net/dhcp/dh4client.hpp>
#include <sstream>

const char* service_name__ = "...";
std::unique_ptr<net::Inet4<VirtioNet> > inet;

#include <memdisk>
#include <fs/fat.hpp> // FAT32 filesystem
using namespace fs;

// assume that devices can be retrieved as refs with some kernel API
// for now, we will just create it here
MemDisk device;

// describe a disk with FAT32 mounted on partition 0 (MBR)
using MountedDisk = fs::Disk<FAT32>;
// disk with filesystem
std::unique_ptr<MountedDisk> disk;

void Service::start()
{
  // networking stack
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0);
  inet->network_config(
      {{  10,  0,  0, 42 }},   // IP
			{{ 255,255,255,  0 }},   // Netmask
			{{  10,  0,  0,  1 }},   // Gateway
			{{   8,  8,  8,  8 }} ); // DNS
  
  // instantiate disk with filesystem
  disk = std::make_unique<MountedDisk> (device);
  
  // list partitions
  disk->partitions(
  [] (fs::error_t err, auto& parts)
  {
    if (err)
    {
      printf("Failed to retrieve volumes on disk\n");
      return;
    }
    
    for (auto& part : parts)
    {
      printf("* Volume: %s at LBA %u\n",
          part.name().c_str(), part.lba_begin);
    }
  });
  
  // mount auto-detected partition
  disk->mount(
  [] (fs::error_t err)
  {
    if (err)
    {
      printf("Could not mount filesystem\n");
      return;
    }
    
    disk->fs().ls("/",
    [] (fs::error_t err, FileSystem::dirvec_t ents)
    {
      if (err)
      {
        printf("Could not list '/Sample Pictures' directory\n");
        return;
      }
      
      for (auto& e : *ents)
      {
        printf("%s: %s\t of size %llu bytes (CL: %llu)\n",
          e.type_string().c_str(), e.name.c_str(), e.size, e.block);
        
        if (e.type == FileSystem::FILE)
        {
          printf("*** Attempting to read: %s\n", e.name.c_str());
          disk->fs().readFile(e,
          [e] (fs::error_t err, fs::buffer_t buffer, size_t len)
          {
            if (err)
            {
              printf("Failed to read file %s!\n",
                  e.name.c_str());
              return;
            }
            
            std::string contents((const char*) buffer.get(), len);
            printf("[%s contents]:\n%s\nEOF\n\n", 
                e.name.c_str(), contents.c_str());
          });
        }
      }
    });
    
    disk->fs().stat("/test.txt",
    [] (fs::error_t err, const FileSystem::Dirent& e)
    {
      if (err)
      {
        printf("Could not stat /TEST.TXT\n");
        return;
      }
      
      printf("stat: /test.txt is a %s on cluster %llu\n", 
          e.type_string().c_str(), e.block);
    });
    disk->fs().stat("/Sample Pictures/Koala.jpg",
    [] (fs::error_t err, const FileSystem::Dirent& e)
    {
      if (err)
      {
        printf("Could not stat /Sample Pictures/Koala.jpg\n");
        return;
      }
      
      printf("stat: /Sample Pictures/Koala.jpg is a %s on cluster %llu\n", 
          e.type_string().c_str(), e.block);
    });
    
    /*
    net::TCP::Socket& sock =  inet->tcp().bind(80);
    sock.onAccept([] (net::TCP::Socket& conn)
    {
      std::string req = conn.read(1024);
      
      printf("SERVICE got data: %s \n", req.c_str());
      
      if (req.find("smaller2.jpg") != req.npos)
      {
        printf("Looking for image file\n");
        
        disk->fs().readFile("/test/smaller2.jpeg",
        [&conn] (fs::error_t err, const uint8_t* buffer, size_t len)
        {
          if (err)
          {
            printf("Failed to read file!\n");
            std::string header="HTTP/1.1 404 Not Found\n"				\
              "Date: Mon, 01 Jan 1970 00:00:01 GMT\n"			\
              "Server: IncludeOS prototype 4.0\n"				\
              "Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT\n"		\
              "Content-Type: text/html; charset=UTF-8\n"			\
              "Content-Length: 0\n"		\
              "Accept-Ranges: bytes\n"					\
              "Connection: close\n\n";
            
            conn.write(header);
            return;
          }
          
          printf("Sending linux penguin image...\n");
          std::string header=
            "HTTP/1.1 200 OK\n "				\
            "Date: Mon, 01 Jan 1970 00:00:01 GMT\n"			\
            "Server: IncludeOS prototype 4.0 \n"				\
            "Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT\n"		\
            "Content-Type: image/jpeg\n"			\
            "Content-Length: "+std::to_string(len)+"\n"		\
            "Accept-Ranges: bytes\n"					\
            "Connection: close\n\n";
          
          conn.write(header);
          conn.write( std::string((const char*) buffer, len) );
        });
      }
      else
      {
        //// generate webpage ////
        uint32_t color = rand();
        
        // HTML Fonts
        std::string ubuntu_medium  = "font-family: \'Ubuntu\', sans-serif; font-weight: 500; ";
        std::string ubuntu_normal  = "font-family: \'Ubuntu\', sans-serif; font-weight: 400; ";
        std::string ubuntu_light  = "font-family: \'Ubuntu\', sans-serif; font-weight: 300; ";
        
        // HTML
        std::stringstream html;
        html << "<html><head>"
         << "<link href='https://fonts.googleapis.com/css?family=Ubuntu:500,300' rel='stylesheet' type='text/css'>"
         << "</head><body>"
         << "<h1 style= \"color: " << "#" << std::hex << (color >> 8) << "\">"	
         << "<span style=\""+ubuntu_medium+"\">Include</span><span style=\""+ubuntu_light+"\">OS</span> </h1>"
         << "<h2>Now speaks TCP!</h2>"
         << "<img src='smaller2.jpg' />"
         << "<p>  ...and can improvise http. With limitations of course, but it's been easier than expected so far </p>"
         << "<footer><hr /> &copy; 2015, Oslo and Akershus University College of Applied Sciences </footer>"
         << "</body></html>\n";
        
        html.seekg(0, std::ios::end);
        
        // HTTP-header
        std::string header=
          "HTTP/1.1 200 OK \n "				\
          "Date: Mon, 01 Jan 1970 00:00:01 GMT \n"			\
          "Server: IncludeOS prototype 4.0 \n"				\
          "Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT \n"		\
          "Content-Type: text/html; charset=UTF-8 \n"			\
          "Content-Length: "+std::to_string(html.tellg())+"\n"		\
          "Accept-Ranges: bytes\n"					\
          "Connection: close\n\n";
        
        conn.write(header);
        conn.write(html.str());
        
        // We don't have to actively close when the http-header says "Connection: close"
        //conn.close();
      }
      
    }); // socket.onConnect */
    
  }); // disk->auto_detect()
  
  printf("*** TEST SERVICE STARTED *** \n");
}