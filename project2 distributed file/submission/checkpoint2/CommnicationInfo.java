import java.io.Serializable;

public class CommnicationInfo implements Serializable{
    private String pathName;
    private int exception;
    private byte[] bytes;
    private String mode;
    private FileHandling.OpenOption openOption;
    private boolean delete;

    public CommnicationInfo(){
    }

    public String getPathName(){
        return this.pathName;
    }

    public int getException(){
        return this.exception;
    }

    public byte[] getBytes(){
        return this.bytes;
    }

    public String getMode(){
        return this.mode;
    }

    public FileHandling.OpenOption getOpenOption(){
        return this.openOption;
    }

    public boolean getDelete(){
        return this.delete;
    }

    public void setPathName(String pathName){
        this.pathName = pathName;
    }

    public void setException(int e){
        this.exception = e;
    }

    public void setBytes(byte[] bytes){
        this.bytes = bytes;
    }

    public void setMode(String mode){
        this.mode = mode;
    }

    public void setOpenOption(FileHandling.OpenOption o){
        this.openOption = o;
    }
    
    public void setDelete(boolean delete){
        this.delete = delete;
    }
}
