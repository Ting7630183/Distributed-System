import java.rmi.NotBoundException;
import java.rmi.RemoteException;
import java.rmi.registry.LocateRegistry;
import java.rmi.registry.Registry;

import java.io.*;
import java.net.Socket;
import java.nio.file.Files;
import java.nio.file.OpenOption;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Map;
import java.util.Set;

import java.util.HashMap;
import java.util.HashSet;

/**
 * This a proxy class that will store file in a cache, if cache hit, file will directly given to client,
 * if not, proxy will fetch file from the server.
 */
class Proxy {
    private static Cache cache = new Cache();
    private static RMIInterface server;
    
    /**
     * The FileHandling class to handle open, read, write, close, unlink methods for file. 
     */
	private static class FileHandler implements FileHandling {
        private Map<Integer, RandomAccessFile> fdRandomAccessFileMap = new HashMap<>();
        private  Map<RandomAccessFile, String> permissionMap = new HashMap<>();
        private Map<RandomAccessFile, File> randomAcessFileToFileMap = new HashMap<>();
        private int fd = 1024;
        
        /**
         * Open and possibly create a file
         * @param path the path name.
         * @param o the open option of a file.
         * @return On success, return the new file descriptor (a nonnegative integer).  
                   On error, -1 is returned and errno is set to indicate the error.
         */
		public int open( String path, OpenOption o) {
            try {
                CommnicationInfo infoFromCache = cache.cacheOpenHelper(path, o);
                File newFile = infoFromCache.getFile();
                String mode = infoFromCache.getMode();
                RandomAccessFile randomAccessFile = new RandomAccessFile(newFile, mode);
                fd++;
                fdRandomAccessFileMap.put(fd, randomAccessFile);
                boolean isDirectory = newFile.isDirectory();
                if(isDirectory && !(mode == "r")){
                    return Errors.EISDIR;
                }
                // pathFileMap.put(path, newFile);
                permissionMap.put(randomAccessFile, mode); 
                randomAcessFileToFileMap.put(randomAccessFile, newFile);             
                return fd;
                
            } catch (IllegalArgumentException e1) {
                e1.printStackTrace();
				return -1;         
			} catch (FileNotFoundException e2) {
                e2.printStackTrace();
				return -1;
			} catch (SecurityException e3){
                e3.printStackTrace();
				return -1;
			} 
		}

        private void copyFileToCache(String path, byte[] bytes){
            if(path != null && bytes != null){
                try {FileOutputStream fos = new FileOutputStream(path);
                    fos.write(bytes);  
                } catch (FileNotFoundException e) {
                    e.printStackTrace();
                } catch (IOException ee){
                    ee.printStackTrace();
                }
            }            
        }

        /**
         * Close a file.
         * @param fd the file descriptor of a file.
         * @return returns zero on success.  On error, -1 is returned, and
                   errno is set to indicate the error.
         */
		public synchronized int close( int fd ) {
			try {
				RandomAccessFile randomAccessFile = fdRandomAccessFileMap.get(fd);
                File file = randomAcessFileToFileMap.get(randomAccessFile);
                if(file.isDirectory()){
                    return Errors.EISDIR;
                }
				if(!(file.exists())){
					return Errors.EBADF;
				}
			    randomAccessFile.close();

                /**When the file closes, push the modification to the server. */
                boolean modify = cache.getModified().containsKey(fd);
                if(modify){
                    File modifiedFile = cache.getModified().get(fd);
                    cache.pushToServer(modifiedFile);
                }
			    return 0;
			}catch (IOException e) {
                e.printStackTrace();
				return -1;           
            }   		
		}

        /**
         * Write to a file descriptor
         * @param fd the file descriptor of a file.
         * @param buf the buffer
         * @return On success, the number of bytes read is returned.
         *         On error, -1 is returned, and errno is set to indicate the error. 
         */
		public synchronized long write( int fd, byte[] buf ) {	
            try {
                RandomAccessFile randomAccessFile = fdRandomAccessFileMap.get(fd);
                File file = randomAcessFileToFileMap.get(randomAccessFile);
                if(file.isDirectory()){
                    return Errors.EISDIR;
                }
                //check fd is valid or not.
                if(!(file.exists())){
					return Errors.EBADF;
                }

                //check permission is valid or not.
                String permission = permissionMap.get(randomAccessFile);
                if(permission == "r"){
					return Errors.EBADF;
                }
                randomAccessFile.write(buf);
				long size = (long) buf.length;
                cache.getModified().put(fd, file); //put the modified file to the modifiedMap. 
			    return size;    

            } catch (IOException e) {
                e.printStackTrace();
				return -1; 
			}	
		}

        /**
         * Read from a file descriptor
         * @param fd the file descriptor of a file.
         * @param buf the buffer
         * @return On success, the number of bytes read is returned.
         *         On error, -1 is returned, and errno is set to indicate the error. 
         */
		public long read( int fd, byte[] buf ) {
            try {
				RandomAccessFile randomAccessFile = fdRandomAccessFileMap.get(fd);
                File file = randomAcessFileToFileMap.get(randomAccessFile);
                if(file.isDirectory()){
                    return Errors.EISDIR;
                }
				if(!(file.exists())){
					return Errors.EBADF;
				}
				int result = randomAccessFile.read(buf);
				if(result == -1){
					return 0;
				}
				return result;

			}  catch (NullPointerException e1) {
                e1.printStackTrace();
				return -1;
			} catch (IOException e2) {
                e2.printStackTrace();
				return -1;
			}	        
		}
        
        /**
         * Reposition read/write file offset
         * @param fd the fd of a file.
         * @param pos the position to be found.
         * @param o open option of a file.
         * @return Upon successful completion, lseek() returns the resulting offset
                   location as measured in bytes from the beginning of the file.  On
                   error, the value (off_t) -1 is returned and errno is set to
                   indicate the error.
         */
		public long lseek( int fd, long pos, LseekOption o ) {
            try {
                RandomAccessFile randomAccessFile = fdRandomAccessFileMap.get(fd);
                File f = randomAcessFileToFileMap.get(randomAccessFile);
                if(f.isDirectory()){
                    return Errors.EISDIR;
                }
				if(!(f.exists())){
			        return Errors.EBADF;
				}
                File file = randomAcessFileToFileMap.get(randomAccessFile);
                long lengthOfFile = file.length();
				long position = 0;
				long currentPosition = randomAccessFile.getFilePointer();
				switch(o){
					case FROM_START:
					    position = pos;
                        break;
					case FROM_CURRENT:
					    position = currentPosition + pos;
                        break;
					case FROM_END:
					    position = lengthOfFile - pos;
                        break;
				}
				if(position < 0){
					return Errors.EBADF;
				}
                randomAccessFile.seek(position);
				return position;    
            } catch (IOException e) {
                e.printStackTrace();
				return -1;
			}	
		}
        
        /**
         * Delete a name and possibly the file it refers
         * @param path the name of a path.
         * @return On success, zero is returned.  On error, -1 is returned, and
                   errno is set to indicate the error.
         */
		public int unlink( String path ) {
            try {
                String pathConverted = cache.convertPath(path);
                File file = new File(pathConverted);

                if(file.isDirectory()){
                    return Errors.EISDIR;
                }

                if(!file.exists()){
                    return Errors.ENOENT;
                }
                sendDelete(path);
                boolean result = file.delete();
                return 0;   
            } catch (SecurityException e) {
                e.printStackTrace();
                return -1;
            }
		}

    
        public void sendDelete(String path){
            try {
                CommnicationInfo infoToServer = new CommnicationInfo();
                infoToServer.setPathName(path);
                infoToServer.setDelete(true);
                server.push(infoToServer);   
            } catch (Exception e) {
                e.printStackTrace();
            }  
        }

		public void clientdone() {
			return;	
		}
	}
	
    /**
     * A Factory class that implements FileHandlingMaking.
     */
	private static class FileHandlingFactory implements FileHandlingMaking {
        /**
         * Create a thread when a new client comes in.
         * @return a FileHandler.
         */
		public FileHandling newclient() {
			return new FileHandler();
		}
	}

    
    /**
     * The main method in proxy.
     * @param args an array in which are three elemets including hostIp, port, cacheDirectory and cache size.
     * @throws IOException IOException.
     */
	public static void main(String[] args) throws IOException {
        String hostIp = "127.0.0.1";
        int port = 1234;
        String cacheDirectory = " ";
        int cacheSize = 0;

        if(args != null && args.length != 0){
            hostIp = args[0];
            port = Integer.parseInt(args[1]);
            cacheDirectory = args[2];
            cacheSize = Integer.parseInt(args[3]);   
        }
        cache.register(hostIp, port);
        cache.createCacheDirectory(cacheDirectory);
        cache.setCacheSize(cacheSize);
		(new RPCreceiver(new FileHandlingFactory())).run();
        
	}
}


