import java.io.File;

import com.twitter.corpus.data.JsonStatusBlockReader;
import com.twitter.corpus.data.JsonStatusCorpusReader;
import com.twitter.corpus.data.Status;
import com.twitter.corpus.data.StatusStream;

import ivory.core.tokenize.GalagoTokenizer;

public class ParseTweets2011 {
  public static void main(String[] args) throws Exception {
    GalagoTokenizer tokenizer = new GalagoTokenizer();
    String input = args[0];

    StatusStream stream;
    File file = new File(input);
    if (file.isDirectory()) {
      stream = new JsonStatusCorpusReader(file);
    } else {
      stream = new JsonStatusBlockReader(file);
    }

    Status status;
    int cnt = 0;
    while ((status = stream.next()) != null) {
      String text = status.getText();
      if (text == null) {
        continue;
      }
      cnt++;
      String[] tokens = tokenizer.processContent(text);
      if(tokens == null) continue;
      String doc = "";
      int length = 0;
      for(String t: tokens) {
        if(t.matches("[a-zA-Z_0-9-]*")) {
          doc += t + " ";
          length++;
        }
      }
      if(length > 0) {
        System.out.println(cnt + "\t" + doc);
      }
    }
  }
}
