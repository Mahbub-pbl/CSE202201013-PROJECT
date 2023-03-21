import 'package:flutter/material.dart';
import 'Pages/face_recog.dart';
import 'Pages/face_registrer.dart';
import 'Pages/hompage.dart';
import 'Pages/loginpage.dart';
import 'Pages/watch_stream.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Flutter Demo',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        primarySwatch: Colors.blue,
      ),
      routes: {
        'lp': (_) => const LogInPage(title: 'Face Detecting and Recognizing App'),
        'hp': (_) => const MyHomePage(title: 'Homepage'),
        'ws': (_) => WatchStreamPage(title: 'Watch Stream', ip: ''),
        'freg': (_) => FaceRegisterPage(title: 'Face Register', ip: ''),
        'frecog': (_) =>
            FaceRecognitionPage(title: 'Face Recognition Page', ip: ''),
      },
      initialRoute: 'lp',
    );
  }
}
