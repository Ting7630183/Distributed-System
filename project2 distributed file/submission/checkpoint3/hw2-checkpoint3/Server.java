import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.rmi.RemoteException;
import java.rmi.registry.LocateRegistry;
import java.rmi.registry.Registry;
import java.rmi.server.UnicastRemoteObject;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * A server that store all the file in local.
 */
public class Server extends UnicastRemoteObject implements RMIInterface {
    private static String fileroot;
    
    /**
     * Constructor of a Server.
     * @param port the port used to communicate with proxy.
     * @throws RemoteException remoteException.
     */
    public Server(int port) throws RemoteException{
        super(port);
    }

    
    /**
     * Convert the path from proxy into a local path.
     * @param path the path of a file from proxy.
     * @return the path of the file in local.
     */
    private String convertPath(String path){
        String newPath = fileroot + "/" + path;
        return newPath;
    }

    /**
     * Check whether a file in the proxy cache is up to date. if yes, send a boolean value back, if not,
     * send the latest version of the file back to proxy.
     */
    public CommnicationInfo check(CommnicationInfo info) throws RemoteException{
        try {
            String path = info.getPathName();
            File file = new File(convertPath(path));


            FileHandling.OpenOption o = info.getOpenOption();
            boolean exist = file.exists();
            String mode = " ";

            CommnicationInfo back = new CommnicationInfo();
            int e = 0; 
            
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
            //check whether file exist or not when OpenOption is CREATE_NEW.
            if(mode == "create"){
                if(exist){
                    e = FileHandling.Errors.EEXIST;
                }else{                  
                    mode = "rw";
                }
            }
            //check whether file exist or not when OpenOption is WRITE.
            if(mode == "write"){
                //!file.exist()
                if(!exist){
                    e = FileHandling.Errors.ENOENT; // not sure about this.
                }else{
                    mode = "rw";
                }
            }
            if(mode == "r"){
                if(!exist){
                    e = FileHandling.Errors.ENOENT; // not sure about this.
                }
            }
            back.setException(e);
            back.setMode(mode);
            back.setPathName(path);

            if(exist == true){
                String localPath = convertPath(path);
                File copyBackFile = new File(localPath);
                long size = copyBackFile.length();
                byte[] bytes = new byte[(int)size];

                try(FileInputStream fis = new FileInputStream(localPath)) {
                    fis.read(bytes);
                } catch (Exception ee) {
                    ee.printStackTrace();
                }  
                back.setBytes(bytes);
            }
            return back;
            
        } catch (Exception e) {
            e.printStackTrace();
        }
        return new CommnicationInfo();
        
    }

    /**
     * Push the file modified by proxies to server.
     */
    public void push(CommnicationInfo info) {
        byte[] bytes = info.getBytes();
        String path = info.getPathName();
        String convertedPath = convertPath(path);
        File file = new File(convertedPath);

        boolean delete = info.getDelete();
            if(delete){
                file.delete();
                return;
            }
       
        if(!file.exists()){
            try {
                file.createNewFile();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        try(FileOutputStream fos = new FileOutputStream(convertedPath)) {
            fos.write(bytes);
        } catch (Exception e) {
            e.printStackTrace();
        }  

    }
    
    /**
     * The main method of Server.
     * @param args args[0] is the port, args[1] is fileroot of the server cache directory of files.
     */
    public static void main(String[] args){
        fileroot = args[1];
        File theDir = new File(fileroot);
        boolean res = false;
        if (!theDir.exists()){
            res = theDir.mkdirs();
        }
        String newPathName = fileroot +  "/" + "haha";
        File foooo = new File(newPathName);
        try {
            foooo.createNewFile();
        } catch (Exception e) {
            e.printStackTrace();
        }

        int port = Integer.parseInt(args[0]);
        try {
            Server server = new Server(port);
            Registry registry = LocateRegistry.createRegistry(port);
            registry.bind("RMIInterface", server);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    
}
