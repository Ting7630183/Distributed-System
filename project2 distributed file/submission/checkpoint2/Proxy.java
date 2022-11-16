
/* Sample skeleton for proxy */
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


class Proxy {
    private static RMIInterface server;
    private static String cacheDirectory;
    // private static int cacheSize;

	private static class FileHandler implements FileHandling {
        private Map<Integer, RandomAccessFile> fdRandomAccessFileMap = new HashMap<>();
        private  Map<RandomAccessFile, String> permissionMap = new HashMap<>();
        private Map<RandomAccessFile, File> randomAcessFileToFileMap = new HashMap<>();
        private int fd = 1024;
    
        Map<Integer, File> modified = new HashMap<>();
        Map<Integer, String> fdPathMap = new HashMap<>();
        

        private String convertPath(String path){
            String newPath = cacheDirectory + "/" + path;
            return newPath;
        }

        private void createDirectory(){
            boolean res = false;
            System.out.println("the cacheDirectory in the open method is: " + cacheDirectory);
            File theDir = new File(cacheDirectory);
            if (!theDir.exists()){
                 res = theDir.mkdirs();
                 System.out.println("the result of creating a directory is: " + res);
            }
        }

        private void createHelper(OpenOption o, String path){
            String m;
            switch (o){
                case CREATE:
                    m = "CREATE";
                    break;
                default:
                m = "OTHER";
                break;
            }

            if(m == "CREAT"){
                File file = new File(path);
                pushToServer(file);
                System.out.println("enter pushtoServer in open.........");
            }
        }
        

		public int open( String path, OpenOption o) {
            createDirectory();
            
            System.out.println("enter open");	
            System.out.println("the OpenOption is open is: " + o);
            System.out.println("the path is: " + path);
            System.out.println("just enter open, fd is: " + fd);

            String pathInProxy = convertPath(path);
            System.out.println("the path that is converted in proxy is......: " + pathInProxy);
            
            try {
                System.out.println("Enter try............. ");
                createHelper(o, path);
                boolean fileExistInCache = false;
                fileExistInCache = new File(cacheDirectory, path).exists();
                
                System.out.println("the path exitence is.....: " + fileExistInCache);
                File newFile;
                String mode = " ";

                if(!fileExistInCache ){
                    System.out.println("enter file does not exist branch........");
                    CommnicationInfo infoToServer = new CommnicationInfo();
                    infoToServer.setOpenOption(o);
                    infoToServer.setPathName(path);
                    CommnicationInfo infoback = server.check(infoToServer);
                    System.out.println("get the return from server...");

                    int except = infoback.getException();
                    if(except < 0){
                        return except;
                    }

                    mode = infoback.getMode();
                    String pathPath = infoback.getPathName();
                    System.out.println("the pathPath value is......" + pathPath);
                    String convetedPathBack = convertPath(pathPath);
                    newFile = new File(convetedPathBack);
                    if(!newFile.exists()){
                        try {
                            newFile.createNewFile();
                        } catch (Exception e) {
                            e.printStackTrace();
                        }    
                    }
                    byte[] bytes = infoback.getBytes();
                    System.out.println("the pathname is:....." + pathPath);
                    System.out.println("g........................");
                    copyFileToCache(convetedPathBack, bytes);

                }else{
                    newFile = new File(convertPath(path));
                    boolean canRead = newFile.canRead();
                    boolean canWrite = newFile.canWrite();
                    if(canRead && canWrite){
                        mode = "rw";
                    }else if(canRead && !(canWrite)){
                        mode = "r";
                    }
                }
                
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
                e1.printStackTrace();
				return -1;         
			} catch (FileNotFoundException e2) {
                e2.printStackTrace();
				return -1;
			} catch (SecurityException e3){
                e3.printStackTrace();
				return -1;
			}  catch(RemoteException e4){
                e4.printStackTrace();
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

		public synchronized int close( int fd ) {
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

                /**When the file closes, push the modification to the server. */
                boolean modify = modified.containsKey(fd);
                if(modify){
                    File modifiedFile = modified.get(fd);
                    pushToServer(modifiedFile);
                }
			    return 0;
			}catch (IOException e) {
				return -1;           
            }   		
		}

        private void pushToServer(File file){
            try {
                byte[] bytes = new byte[(int) file.length()];
                FileInputStream fileInputStream = new FileInputStream(file);
                fileInputStream.read(bytes);
                CommnicationInfo toServer = new CommnicationInfo();
                String path = file.getPath();
                System.out.println("the path name in the pushToServer is:...." + path);
                int lastIndex = path.lastIndexOf('/');
                System.out.println("last index is....." + lastIndex);
                String actualPath = path.substring(lastIndex+1);
                System.out.println("the actual path is....." + actualPath);
                toServer.setPathName(actualPath);
                toServer.setBytes(bytes);
                server.push(toServer);   
            } catch (FileNotFoundException e) {
                return;
            } catch (IOException ee){
                return;
            }
            
        }

		public synchronized long write( int fd, byte[] buf ) {	
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
                modified.put(fd, file); //put the modified file to the modifiedMap. 
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
                String pathConverted = convertPath(path);
                File file = new File(pathConverted);

                if(file.isDirectory()){
                    return Errors.EISDIR;
                }

                if(!file.exists()){
                    return Errors.ENOENT;
                }
               
                sendDelete(path);
                boolean result = file.delete();
                System.out.println("the result of delete is: " + result);
                return 0;
                
            } catch (SecurityException e) {
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
        String hostIp = "127.0.0.1";
        int port = 1234;
        cacheDirectory = " ";
        // cacheSize = 0;

        if(args != null && args.length != 0){
            hostIp = args[0];
            port = Integer.parseInt(args[1]);
            cacheDirectory = args[2];
            System.out.println("the cacheDirectory is: " + cacheDirectory);
            // cacheSize = Integer.parseInt(args[3]);   
        }
        
        try {
            Registry registry = LocateRegistry.getRegistry(hostIp, port);
            server = (RMIInterface) registry.lookup("RMIInterface");
            String response = server.sayHello("client");
            System.out.println("the hello in the proxy is: " + response);
        } catch (RemoteException remote) {
            System.out.println("Unable to locate registry or unable to call RPC sayHello");
            System.exit(1);
        } catch (NotBoundException notbound){
            System.out.println("client not found");
            System.exit(1);
        }

		System.out.println("Hello World");
		(new RPCreceiver(new FileHandlingFactory())).run();
        
	}
}


