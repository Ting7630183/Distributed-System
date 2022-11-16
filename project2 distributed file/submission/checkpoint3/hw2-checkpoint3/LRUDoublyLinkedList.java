import java.util.Map;
import java.util.HashMap;
/**
 * A DoublylinkedList
 */
public class LRUDoublyLinkedList{
    /**
     * A Node structure which stores a pathName, version as well as pointer to previous
     * and next node.
     */

    private Map<String, Node> map; // use HashMap to map pathname with Node.
    // private int size;
    private Node head, tail;

    /**
     * Constructor of the LRUDoublyLinkedList.
     */
    public LRUDoublyLinkedList(){
        map = new HashMap<>();
        // size = 0;
        head = new Node("0", 0);
        tail = new Node("0", 0);
        head.next = tail;
        tail.previous = head;
        head.previous = null;
        tail.next = null;
    }

    /**
     * Delete a node in the doubly linked list.
     * @param node the node to be deleted.
     */
    private void deleteNode(Node node){
        if(node != null){
            node.next.previous = node.previous;
            node.previous.next = node.next;
        }
    }

    /**
     * Add a node to the head of the doubly linked list.
     * @param node the node to be added to the head of the list.
     */
    private void addToHead(Node node){
        node.next = head.next;
        node.next.previous = node;
        node.previous = head;
        head.next = node;
    }

    /**
     * Update the postion of the node with the pathname in the doubly linked list.
     * @param path the pathname.
     * @param version the version number.
     */
    public Node update(String path, int version){
        if(map.containsKey(path)){
            Node node = map.get(path);
            if(version == node.version){
                deleteNode(node);
                addToHead(node);
                return node;
            }else{
                node.version = version;
                deleteNode(node);
                addToHead(node);
                return node;
            }
        }else{
            Node node = new Node(path, version);
            map.put(path, node);
            addToHead(node);
            return node;
        }
    }

    /**
     * Check whether the Node with the path is in the LRUDoublyLinkedList or not.
     * @param path the pathname of a file.
     * @return whether the Node with the path exist or not.
     */
    public boolean exist(String path){
        if(map.containsKey(path)){
            return true;
        }
        return false;
    }

    public int getVersion(String path){
        if(map.containsKey(path)){
            Node node = map.get(path);
            return node.version;
        }
        return -1;
    }
}
