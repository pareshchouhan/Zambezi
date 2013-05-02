package ivory.streaming.driver.hadoop;

import java.io.IOException;
import java.net.URI;
import java.util.Iterator;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.conf.Configured;
import org.apache.hadoop.filecache.DistributedCache;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileUtil;
import org.apache.hadoop.fs.FSDataOutputStream;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.SequenceFile;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.io.compress.GzipCodec;
import org.apache.hadoop.mapred.FileInputFormat;
import org.apache.hadoop.mapred.FileOutputFormat;
import org.apache.hadoop.mapred.JobClient;
import org.apache.hadoop.mapred.JobConf;
import org.apache.hadoop.mapred.MapReduceBase;
import org.apache.hadoop.mapred.Mapper;
import org.apache.hadoop.mapred.Reducer;
import org.apache.hadoop.mapred.Partitioner;
import org.apache.hadoop.mapred.OutputCollector;
import org.apache.hadoop.mapred.Reporter;
import org.apache.hadoop.mapred.SequenceFileInputFormat;
import org.apache.hadoop.mapred.TextOutputFormat;
import org.apache.hadoop.mapred.lib.IdentityReducer;
import org.apache.hadoop.util.Tool;
import org.apache.hadoop.util.ToolRunner;
import org.apache.log4j.Logger;

import edu.umd.cloud9.collection.clue.ClueWarcRecord;
import edu.umd.cloud9.collection.clue.ClueWarcDocnoMapping;
import edu.umd.cloud9.collection.clue.ClueWarcInputFormat;
import edu.umd.cloud9.collection.clue.ClueCollectionPathConstants;

import ivory.core.tokenize.GalagoTokenizer;

@SuppressWarnings("deprecation")
public class ParseClueWebDocuments extends Configured implements Tool {
  private static final Logger sLogger = Logger.getLogger(ParseClueWebDocuments.class);

  private static class MyMapper extends MapReduceBase implements
    Mapper<LongWritable, ClueWarcRecord, IntWritable, Text> {
    private static final IntWritable sDocno = new IntWritable();
    private static final Text outValue = new Text();
    private ClueWarcDocnoMapping mDocnoMapping = new ClueWarcDocnoMapping();
    private GalagoTokenizer tokenizer = new GalagoTokenizer();
    private StringBuffer buffer = new StringBuffer();

    public void configure(JobConf job) {
      Path[] localFiles;
      try {
        localFiles = DistributedCache.getLocalCacheFiles(job);
      } catch (IOException e) {
        throw new RuntimeException("Local cache files not read properly.");
      }

      try {
        mDocnoMapping.loadMapping(localFiles[0], FileSystem.getLocal(job));
      } catch (Exception e) {
        e.printStackTrace();
        throw new RuntimeException("Error initializing DocnoMapping!");
      }
    }

    public void map(LongWritable key, ClueWarcRecord doc,
        OutputCollector<IntWritable, Text> output, Reporter reporter) throws IOException {
      String id = doc.getHeaderMetadataItem("WARC-TREC-ID");

      if (id != null) {
        sDocno.set(mDocnoMapping.getDocno(id));
        String[] terms = tokenizer.processContent(doc.getContent());
        buffer.delete(0, buffer.length());
        for(String t: terms) {
          if(t.matches("[a-zA-Z_0-9-]*")) {
            buffer.append(t + " ");
          }
        }
        outValue.set(buffer.toString());
        output.collect(sDocno, outValue);
      }
    }
  }

  private static class MyPartitioner implements Partitioner<IntWritable, Text> {
    public void configure(JobConf job) {
    }

    public int getPartition(IntWritable key, Text value, int numReduceTasks) {
      int ratio = (int) (50000000/(numReduceTasks));
      int reducer = ((int) key.get() / ratio);
      return reducer >= numReduceTasks ? (numReduceTasks-1) : reducer;
    }
  }

  public int run(String[] args) throws Exception {
    if(args.length != 3) {
      return -1;
    }

    JobConf conf = new JobConf(getConf(), ParseClueWebDocuments.class);
    FileSystem fs = FileSystem.get(conf);

    String inPath = args[0];
    String outPath=args[1];
    String docnoMapping=args[2];

    Path inputPath = new Path(inPath);
    Path outputPath = new Path(outPath);

    if(fs.exists(outputPath)) {
      fs.delete(outputPath);
    }

    conf.setJobName("ParseClueWebDocuments");

    conf.setNumMapTasks(1);
    conf.setNumReduceTasks(500);

    conf.set("mapred.child.java.opts", "-Xmx2048m");
    conf.set("mapred.task.timeout", "36000000");

    DistributedCache.addCacheFile(new URI(docnoMapping), conf);

    ClueCollectionPathConstants.addEnglishCollectionPart(conf, inPath, 1);
    FileOutputFormat.setOutputPath(conf, outputPath);

    conf.setInputFormat(ClueWarcInputFormat.class);
    conf.setOutputFormat(TextOutputFormat.class);
    TextOutputFormat.setCompressOutput(conf, true);
    TextOutputFormat.setOutputCompressorClass(conf, GzipCodec.class);

    conf.setOutputKeyClass(IntWritable.class);
    conf.setOutputValueClass(Text.class);

    conf.setMapperClass(MyMapper.class);
    conf.setPartitionerClass(MyPartitioner.class);
    conf.setReducerClass(IdentityReducer.class);
    JobClient.runJob(conf);
    return 0;
  }

  public static void main(String[] args) throws Exception {
    int res = ToolRunner.run(new Configuration(), new ParseClueWebDocuments(), args);
    System.exit(res);
  }
}
