import org.apache.zookeeper.Watcher;
import org.apache.zookeeper.WatchedEvent;

public class zkWatcher implements Watcher{
    public void process(WatchedEvent event) {
        System.out.println(event);
        String path = event.getPath();
        if (event.getType() == Event.EventType.None) {
            switch (event.getState()) {
                case SyncConnected:
                    System.out.printf("The client is in the connected state\n");
                    break;
                case Expired:
                    System.out.printf("The serving cluster has expired this session.");
                    break;
                default:
                    System.out.println("fck");
            }
        } else {
            if (path != null) {
                System.out.printf("Something has changed on the node, path %s\n", path);
            }
        }
    }
}
