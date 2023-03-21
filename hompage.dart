import 'package:flutter/material.dart';
import 'package:flutter_face_recog2/Pages/watch_stream.dart';
import 'face_recog.dart';
import 'face_registrer.dart';

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});
  final String title;
  @override
  State<MyHomePage> createState() => _MyHomePageState();
}
class _MyHomePageState extends State<MyHomePage> {
  final TextEditingController tEC1 = TextEditingController();

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.title),
      ),
      body: Center(
          child: SingleChildScrollView(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            SizedBox(
              height: 100,
              width: 300,
              child: Row(children: [
                SizedBox(
                  height: 500, width: 300,
                  child: TextFormField(
                      controller: tEC1,
                      style: const TextStyle(
                        fontSize: 18,
                        color: Colors.black,
                      ),
                      decoration: const InputDecoration(
                          hintText: "IP Address",
                          hintStyle: TextStyle(
                            fontSize: 18,
                            color: Colors.grey,
                          ))),
                ),
              ]),
            ),
            SizedBox(
                height: 50,
                width: 250,
                child: ElevatedButton(
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.deepPurpleAccent,
                      elevation: 5,
                      // foreground
                    ),
                    onPressed: () {
                      Navigator.push(
                          context,
                          MaterialPageRoute(
                              builder: (context) => WatchStreamPage(
                                  title: 'Watch Stream', ip: tEC1.text)));
                    },
                    child: const Text('Watch Stream',
                        style: TextStyle(
                          fontWeight: FontWeight.w900,
                          fontSize: 20,
                          color: Colors.white,
                        )))),
            const SizedBox(height: 15),
            SizedBox(
                height: 50,
                width: 250,
                child: ElevatedButton(
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.deepPurpleAccent,
                      elevation: 5,
                      // foreground
                    ),
                    onPressed: () {
                      Navigator.push(
                          context,
                          MaterialPageRoute(
                              builder: (context) => FaceRegisterPage(
                                  title: 'Face Register', ip: tEC1.text)));
                    },
                    child: const Text('Face Register',
                        style: TextStyle(
                          fontWeight: FontWeight.w900,
                          fontSize: 20,
                          color: Colors.white,
                        )))),
            const SizedBox(height: 15),
            SizedBox(
                height: 50,
                width: 250,
                child: ElevatedButton(
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.deepPurpleAccent,
                      elevation: 5, // foreground
                    ),
                    onPressed: () {
                      Navigator.push(
                          context,
                          MaterialPageRoute(
                              builder: (context) => FaceRecognitionPage(
                                  title: 'Face Recognition', ip: tEC1.text)));
                    },
                    child: const Text('Face Recognition',
                        style: TextStyle(
                          fontWeight: FontWeight.w900,
                          fontSize: 20,
                          color: Colors.white,
                        )))),
          ],
        ),
      )),
      // This trailing comma makes auto-formatting nicer for build methods.
    );
  }
}
