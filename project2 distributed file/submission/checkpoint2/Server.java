
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


public class Server extends UnicastRemoteObject implements RMIInterface {
    private static String fileroot;
    // Set<String> serverCache = new HashSet<>();

    public Server(int port) throws RemoteException{
        super(port);
        System.out.println("come into the constructor of the server");
    }

    public String sayHello(String name){
        System.out.println("hello in the server");
        return "hello";
    }

    
    private String convertPath(String path){
        String newPath = fileroot + "/" + path;
        return newPath;
    }
    public CommnicationInfo check(CommnicationInfo info) throws RemoteException{
        System.out.println("enter check..........");
        try {
            String path = info.getPathName();
            System.out.println("the path get in the server is:...." + path);
            File file = new File(convertPath(path));


            FileHandling.OpenOption o = info.getOpenOption();
            System.out.println("the OpenOption in the server is:...." + o);
            boolean exist = file.exists();
            System.out.println("the file exist or not is:...." + exist);
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
            System.out.println("the mode after the first switch is: " + mode);
            //check whether file exist or not when OpenOption is CREATE_NEW.
            if(mode == "create"){
                if(exist){
                    System.out.println("open CREATE_NEW is not match, return error of EEXIST");
                    e = FileHandling.Errors.EEXIST;
                }else{                  
                    mode = "rw";
                }
            }
            //check whether file exist or not when OpenOption is WRITE.
            if(mode == "write"){
                //!file.exist()
                if(!exist){
                    System.out.println("open, the file of WRITE is not exist, return error of ENOENT");
                    e = FileHandling.Errors.ENOENT; // not sure about this.
                }else{
                    mode = "rw";
                }
            }
            if(mode == "r"){
                if(!exist){
                    System.out.println("open, the file of read is not exist, return error of ENOENT");
                    e = FileHandling.Errors.ENOENT; // not sure about this.
                }
            }
            System.out.println("the exception after checking is:...." + e);
            back.setException(e);
            back.setMode(mode);
            back.setPathName(path);

            if(exist == true){
                System.out.println("enter to the branch that the file exist");
                String localPath = convertPath(path);
                File copyBackFile = new File(localPath);
                long size = copyBackFile.length();
                System.out.println("the number of bytes copy back is......." + size);
                byte[] bytes = new byte[(int)size];
                System.out.println("the byte copy back is......." + bytes);

                try(FileInputStream fis = new FileInputStream(localPath)) {
                    fis.read(bytes);
                } catch (Exception ee) {
                    System.out.println("exception");
                }  
                back.setBytes(bytes);
            }
            return back;
            
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
        
    }

    public void push(CommnicationInfo info) {
        System.out.println("enter push method in server.......");
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

    public static void main(String[] args){
        fileroot = args[1];
        System.out.println("fileroot in the server is: " + fileroot);
        File theDir = new File(fileroot);
        boolean res = false;
        if (!theDir.exists()){
            res = theDir.mkdirs();
        }
        System.out.println("the result of creating a directory in server is: " + res);
        String newPathName = fileroot +  "/" + "haha";
        File foooo = new File(newPathName);
        try {
            foooo.createNewFile();
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        System.out.println("the fooooo exist or not:" + foooo.exists());
        
        System.out.println("Enter the server");
        int port = Integer.parseInt(args[0]);
        try {
            System.out.println("the port is: " + port);
            Server server = new Server(port);
            server.sayHello("SDf");
            System.out.println("1...............");
            Registry registry = LocateRegistry.createRegistry(port);
            System.out.println("2...............");
            registry.bind("RMIInterface", server);
            System.out.println("Server ready");

        } catch (Exception e) {
            System.out.println("server exception");
            e.printStackTrace();
        }
    }

    
}
