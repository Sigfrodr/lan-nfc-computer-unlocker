<?php
return [
    'settings' => [
        'displayErrorDetails' => true, // set to false in production
        'addContentLengthHeader' => false, // Allow the web server to send the content-length header
        
        // Twig settings
        'view' => [
            'template_path' => __DIR__ . '/../templates/',
            'cache_path' => false
        ],

        // Monolog settings
        'logger' => [
            'name' => 'slim-app',
            'path' => __DIR__ . '/../logs/app.log',
            'level' => \Monolog\Logger::DEBUG,
        ],
        
        // Database connection settings
        'db' => [
            'host' => '127.0.0.1',
            'dbname' => 'lancer',
            'user' => 'root',
            'pass' => ''
        ],
        
        // Lancer settings
        'lancer' => [
            'key' => 'lancer'
        ],
    ],
];
