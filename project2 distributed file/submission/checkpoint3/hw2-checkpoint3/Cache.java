import java.io.File;
import java.rmi.NotBoundException;
import java.rmi.RemoteException;
import java.rmi.registry.LocateRegistry;
import java.rmi.registry.Registry;
import java.util.HashMap;
import java.util.Map;
import java.io.*;
import java.net.Socket;
import java.nio.file.Files;
import java.nio.file.OpenOption;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Set;
import java.util.HashSet;

/**
 * Cache is a class to do all the cache work inlcuding checking whether the file is in the cache,
 * update the cache file and etc.
 */
public class Cache {
    Map<Integer, File> modified = new HashMap<>();
    Map<Integer, String> fdPathMap = new HashMap<>();
    private String cacheDirectory;
    private int cacheSize;
    private RMIInterface server;
    private LRUDoublyLinkedList LRU = new LRUDoublyLinkedList();
    public Cache(){
    }

    /**
     * Make registration to connect to the server.
     * @param hostIp the hostIp address.
     * @param port the port.
     */
    public void register(String hostIp, int port){
        try {
            Registry registry = LocateRegistry.getRegistry(hostIp, port);
            server = (RMIInterface) registry.lookup("RMIInterface");
            String response = server.sayHello("client");
        } catch (RemoteException remote) {
            remote.printStackTrace();
        } catch (NotBoundException notbound){
            notbound.printStackTrace();
        }
    }
    
    /**
     * Create a cacheDirectory if it does not exist yet.
     * @param cacheDirectory the name of the directory.
     */
    public void createCacheDirectory(String cacheDirectory){
        this.cacheDirectory = cacheDirectory;
        File theDir = new File(cacheDirectory);
        if (!theDir.exists()){
            theDir.mkdirs();
        } 
   }
    
   /**
    * Convert the pathName to the one in the cache.
    * @param path the pathname from client.
    * @return the pathname in the cache.
    */
    public String convertPath(String path){
        String newPath = cacheDirectory + "/" + path;
        return newPath;
    }
    
    /**
     * Get the name of the cache directory.
     */
    public String getCacheDirectory(){
        return this.cacheDirectory;
    }

    /**
     * Get the map which map the file fd with the file.
     * @return
     */
    public Map<Integer, File> getModified(){
        return this.modified;
    }

    /**
     * Get the cache size.
     * @param size the size of the cache.
     */
    public void setCacheSize(int size){
        this.cacheSize = size;   
    }

    /**
     * The helper method to deal with the situation that the open creates a new file.
     * @param o OpenOption
     * @param path the pathname of a file.
     */
    public void createHelper(FileHandling.OpenOption o, String path){
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
        }
    }

    /**
     * A helper function to push the file to the server.
     * @param file the name of a file.
     */
    public void pushToServer(File file){
        try {
            byte[] bytes = new byte[(int) file.length()];
            try (FileInputStream fileInputStream = new FileInputStream(file)) {
                fileInputStream.read(bytes);
            }
            CommnicationInfo toServer = new CommnicationInfo();
            String path = file.getPath();
            int lastIndex = path.lastIndexOf('/');
            String actualPath = path.substring(lastIndex+1);
            toServer.setPathName(actualPath);
            toServer.setBytes(bytes);
            server.push(toServer);   
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return;
        } catch (IOException ee){
            ee.printStackTrace();
            return;
        }           
    }
    
    /**
     * The helper method to deal with open.
     * @param path the pathname of a file.
     * @param o the Open Option of a file.
     * @return CommunicationInfo.
     */
    public CommnicationInfo cacheOpenHelper(String path, FileHandling.OpenOption o){ 
            String pathInProxy = convertPath(path);
            
            try {
                createHelper(o, path);
                boolean fileExistInCache = false;
                fileExistInCache = new File(getCacheDirectory(), path).exists();
                File newFile;
                String mode = " ";

                if(!fileExistInCache ){
                    CommnicationInfo infoToServer = new CommnicationInfo();
                    infoToServer.setOpenOption(o);
                    infoToServer.setPathName(path);
                    CommnicationInfo infoback = server.check(infoToServer);
                    int except = infoback.getException();
                    if(except < 0){
                        CommnicationInfo backToProxy = new CommnicationInfo();
                        backToProxy.setException(except);
                        return backToProxy;
                    }

                    mode = infoback.getMode();
                    String pathPath = infoback.getPathName();
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
                CommnicationInfo backToProxy = new CommnicationInfo();
                backToProxy.setMode(mode);
                backToProxy.setFile(newFile);
                return backToProxy;
        }finally{
            return new CommnicationInfo();
        } 
    }
    
    /**
     * Copy the file content the local cache.
     * @param path the pathname of a file.
     * @param bytes byte array.
     */
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
}
