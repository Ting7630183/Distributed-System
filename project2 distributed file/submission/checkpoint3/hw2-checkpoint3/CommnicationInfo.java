import java.io.File;
import java.io.Serializable;
/**
 * This is a dataclass that holds the data transfer between proxy and server.
 */
public class CommnicationInfo implements Serializable{
    private String pathName;
    private int exception;
    private byte[] bytes;
    private String mode;
    private FileHandling.OpenOption openOption;
    private boolean delete;
    private int version;
    private File file;
    
    /**
     * Default constructor.
     */
    public CommnicationInfo(){
    }
    
    /**
     * Get the pathName.
     * @return the pathName.
     */
    public String getPathName(){
        return this.pathName;
    }

    /**
     * Get the exception.
     * @return exception.
     */
    public int getException(){
        return this.exception;
    }

    /**
     * Get bytes that are to be transfer to server or from server.
     */
    public byte[] getBytes(){
        return this.bytes;
    }

    /**
     * Get the mode of a file.
     * @return mode of a file.
     */
    public String getMode(){
        return this.mode;
    }

    /**
     * Get the open option of a file.
     * @return the open option of a file.
     */
    public FileHandling.OpenOption getOpenOption(){
        return this.openOption;
    }

    /**
     * Get the boolean value which tells whether to delete a file.
     * @return a boolean value indicating whether to delete a file.
     */
    public boolean getDelete(){
        return this.delete;
    }
    
    /**
     * Get the file. 
     * @return a Fle.
     */
    public File getFile(){
        return this.file;
    }

    /**
     * Set the pathName.
     * @param pathName the pathName of a file.
     */
    public void setPathName(String pathName){
        this.pathName = pathName;
    }

    /**
     * Set the exception.
     * @param e exception.
     */
    public void setException(int e){
        this.exception = e;
    }

    /**
     * Set the bytes.
     * @param bytes bytes.
     */
    public void setBytes(byte[] bytes){
        this.bytes = bytes;
    }

    /**
     * Set the mode of a file.
     * @param mode the mode of a file.
     */
    public void setMode(String mode){
        this.mode = mode;
    }

    /**
     * Set the open option of a file.
     * @param o the open option of a file.
     */
    public void setOpenOption(FileHandling.OpenOption o){
        this.openOption = o;
    }
    
    /**
     * Set the flag whether the file is to be deleted or not.
     * @param delete boolean value of whether to delete a file.
     */
    public void setDelete(boolean delete){
        this.delete = delete;
    }

    /**
     * Set the file.
     * @param file a File.
     */
    public void setFile(File file){
        this.file = file;
    }
}
