
/* Sample skeleton for proxy */

import java.io.*;
import java.nio.file.OpenOption;
import java.util.Map;
import java.util.Set;
import java.util.HashMap;
import java.util.HashSet;


class Proxy {
	private static class FileHandler implements FileHandling {
        Map<Integer, RandomAccessFile> fdRandomAccessFileMap = new HashMap<>();
        // Map<String, File> pathFileMap = new HashMap<>();
        Map<RandomAccessFile, String> permissionMap = new HashMap<>();
        Map<RandomAccessFile, File> randomAcessFileToFileMap = new HashMap<>();
        int fd = 1024;
		

		public int open( String path, OpenOption o ) {
            System.out.println("enter open");	
            System.out.println("the OpenOption is open is: " + o);
            System.out.println("the path is: " + path);
            System.out.println("just enter open, fd is: " + fd);
            
            try {
                File newFile = new File(path);
                boolean exist = newFile.exists();
                // File file = pathFileMap.get(path);
                // if(file != null){
                //     System.out.println("file already exist, the file is: " + file.toString());
                // }
                // System.out.println("there is no existing file for this path" );
                String mode = " ";
    
                switch (o){
                    case CREATE:
                        mode = "rw";
                        break;
                    case READ:
                        mode = "r";
                        break;
                    case CREATE_NEW:
                        mode = "create";
                        break;
                    case WRITE:
                        mode = "write";
                        break;
                }
                System.out.println("the mode after the first switch is: " + mode);
                //check whether file exist or not when OpenOption is CREATE_NEW.
                if(mode == "create"){
                    if(exist){
                        System.out.println("open CREATE_NEW is not match, return error of EEXIST");
                        return Errors.EEXIST;
                    }else{
                        mode = "rw";
                    }
                }
                //check whether file exist or not when OpenOption is WRITE.
                if(mode == "write"){
                    //!file.exist()
                    if(!exist){
                        System.out.println("open, the file of WRITE is not exist, return error of ENOENT");
                        return Errors.ENOENT; // not sure about this.
                    }else{
                        mode = "rw";
    
                    }
                }
                if(mode == "r"){
                    if(!exist){
                        System.out.println("open, the file of read is not exist, return error of ENOENT");
                        return Errors.ENOENT; // not sure about this.
                    }
                }
                System.out.println("after switch and check, current mode is: " + mode);
                // File newFile = new File(path);
                boolean isFile = newFile.isFile();
                System.out.println("check whether the new file created is a file or not: " + isFile);
                RandomAccessFile randomAccessFile = new RandomAccessFile(newFile, mode);
                fd++;
                fdRandomAccessFileMap.put(fd, randomAccessFile);
                System.out.println("after put the fd in the randomAcessFileMap, fd is: " + fd);
    
                boolean isDirectory = newFile.isDirectory();
                System.out.println("in open, the newFile is Directory or not: " + isDirectory);
                if(isDirectory && !(mode == "r")){
                    return Errors.EISDIR;
                }
                // pathFileMap.put(path, newFile);
                permissionMap.put(randomAccessFile, mode); 
                randomAcessFileToFileMap.put(randomAccessFile, newFile);             
                System.out.println("the fd return is: " + fd);
                return fd;
                
            } catch (IllegalArgumentException e1) {
				return -1;         
			} catch (FileNotFoundException e2) {
				return -1;
			} catch (SecurityException e3){
				return -1;
			}   	
		}

		public int close( int fd ) {
            System.out.println("fd in close is:" + fd);
			try {
				System.out.println("enter close");
				RandomAccessFile randomAccessFile = fdRandomAccessFileMap.get(fd);
                File file = randomAcessFileToFileMap.get(randomAccessFile);
                if(file.isDirectory()){
                    return Errors.EISDIR;
                }
				if(!(file.exists())){
					System.out.println("The error in close is: EBADF");
					return Errors.EBADF;
				}
			    randomAccessFile.close();
			    return 0;
			}catch (IOException e) {
				return -1;           
            }   		
		}

		public long write( int fd, byte[] buf ) {	
            try {
                System.out.println("enter write");
                RandomAccessFile randomAccessFile = fdRandomAccessFileMap.get(fd);
                File file = randomAcessFileToFileMap.get(randomAccessFile);
                if(file.isDirectory()){
                    return Errors.EISDIR;
                }
                //check fd is valid or not.
                if(!(file.exists())){
                    System.out.println("in write, randomAccessFile is null: EBADF");
					return Errors.EBADF;
                }

                //check permission is valid or not.
                String permission = permissionMap.get(randomAccessFile);
                System.out.println("in write the permission is : " + permission);
                if(permission == "r"){
                    System.out.println("in write, the permission is not valid: EBADF");
					return Errors.EBADF;
                }
                randomAccessFile.write(buf);
				long size = (long) buf.length;
				System.out.println("the size has been written is: " + size);
				System.out.println("the content been written is: " + buf);
			    return size;    

            } catch (IOException e) {
				return -1; 
			}	
		}

		public long read( int fd, byte[] buf ) {
            try {
				System.out.println("Enter read");
				System.out.println("the fd in read is: " + fd);
				RandomAccessFile randomAccessFile = fdRandomAccessFileMap.get(fd);
                File file = randomAcessFileToFileMap.get(randomAccessFile);
                if(file.isDirectory()){
                    return Errors.EISDIR;
                }
				if(!(file.exists())){
                    System.out.println("in read, the randomAccessFile is null, return the error of EBADF ");
					return Errors.EBADF;
				}
				int result = randomAccessFile.read(buf);
				if(result == -1){
					System.out.println("the result in read is: 0 ");
					return 0;
				}
				System.out.println("the result in read is: " + result);
                System.out.println("the content that has been read is: " + buf);
				return result;

			}  catch (NullPointerException e1) {
				return -1;
			} catch (IOException e2) {
				return -1;
			}	        
		}

		public long lseek( int fd, long pos, LseekOption o ) {
            System.out.println("enter lseek");
            try {
                System.out.println("enter lseek");
                System.out.println("lseek option is: " + o);
				System.out.println("lseek pos is: " + pos);
				System.out.println("lseek fd is: " + fd);

                RandomAccessFile randomAccessFile = fdRandomAccessFileMap.get(fd);
                File f = randomAcessFileToFileMap.get(randomAccessFile);
                if(f.isDirectory()){
                    return Errors.EISDIR;
                }
				if(!(f.exists())){
					System.out.println("lseek error: EBADF ");
			        return Errors.EBADF;
				}
                File file = randomAcessFileToFileMap.get(randomAccessFile);
                long lengthOfFile = file.length();
				long position = 0;
				long currentPosition = randomAccessFile.getFilePointer();
				switch(o){
					case FROM_START:
                        System.out.println("switch into FROM_START");
					    position = pos;
                        System.out.println("position in FROM_START is: " + position);
                        break;
					case FROM_CURRENT:
                        System.out.println("switch into FROM_CURRENT");
					    position = currentPosition + pos;
                        System.out.println("position in FROM_CURRENT is: " + position);
                        break;
					case FROM_END:
                        System.out.println("switch into FROM_END");
                        System.out.println("position in FROM_END is: " + position);
					    position = lengthOfFile - pos;
                        break;
				}
                System.out.println("the position in lseek is:" + position);
				if(position < 0){
					System.out.println("lseek position is less than 0");
					return Errors.EBADF;
				}
                randomAccessFile.seek(position);
				return position;    
            } catch (IOException e) {
				return -1;
			}	
		}

		public int unlink( String path ) {
            try {
                System.out.println("enter unlink");
				System.out.println("the path is: " + path);
                File file = new File(path);

                if(file.isDirectory()){
                    return Errors.EISDIR;
                }

                if(!file.exists()){
                    return Errors.ENOENT;
                }

                boolean result = file.delete();
                System.out.println("the result of delete is: " + result);
                return 0;
                
            } catch (SecurityException e) {
                return -1;
            }
		}

		public void clientdone() {
			// try {
			// 	for(RandomAccessFile file: allFiles){
			// 		file.close();
			// 	}
			// 	return;
				
			// } catch (IOException e) {
			// 	System.out.println("IOException in clientdone");
			// }
			return;
			
		}

	}
	
	private static class FileHandlingFactory implements FileHandlingMaking {
		public FileHandling newclient() {
			return new FileHandler();
		}
	}

	public static void main(String[] args) throws IOException {
		System.out.println("Hello World");
		(new RPCreceiver(new FileHandlingFactory())).run();
	}
}


