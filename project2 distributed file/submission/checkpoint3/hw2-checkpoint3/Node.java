import java.util.List;

/**
     * A Node structure which stores a pathName, version as well as pointer to previous
     * and next node.
     */
public class Node {
    String pathName;
    int version;
    int reader;
    int writer;
    List<Node> duplicates;
    Node previous;
    Node next;

    /**
     * The constructor of a Node.
     * @param pathName the path name.
     * @param version version number.
     */
    public Node(String pathName, int version){
        this.pathName = pathName;
        this.version = version;
    }

    /**
     * Get the path name of a Node.
     * @return the path name of the Node.
     */
    public String getPathName(){
        return this.pathName;
    }

    /**
     * Get the version number of a Node.
     * @return the version number of a Node.
     */
    public int getVersion(){
        return this.version;
    }

    /**
     * Set the path name of a Node.
     * @param path the path name of a Node.
     */
    public void setPathName(String path){
        this.pathName = path;
    }

    /**
     * Set the version number of a Node.
     * @param version the version number.
     */
    public void setVersion(int version){
        this.version = version;
    }
    
}