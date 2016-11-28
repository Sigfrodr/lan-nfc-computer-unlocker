<?php
// Routes

// Validators
use Respect\Validation\Validator as v;
$nameValidator = v::stringType()->length(1,50);
$cardidValidator = v::numeric()->positive()->length(10, 10);
$passwordValidator = v::stringType()->length(1,256);
$validators = array(
  'name' => $nameValidator,
  'cardid' => $cardidValidator,
  'password' => $passwordValidator
);
$app->add(new \DavidePastore\Slim\Validation\Validation($validators));

// Default
$app->get('/', function ($request, $response, $args) {

    $sth = $this->db->prepare("SELECT * FROM users ORDER BY name");
    $sth->execute();
    $users = $sth->fetchAll();

    return $this->view->render($response, 'index.html', [
        'users' => $users
    ]);
})->setName('root');

// Get all users
/*$app->get('/users', function ($request, $response, $args) {
    $sth = $this->db->prepare("SELECT * FROM users ORDER BY name");
    $sth->execute();
    $users = $sth->fetchAll();
    return $this->response->withJson($users);
});*/

// Retrieve user with cardid 
$app->get('/user/{cardid}', function ($request, $response, $args) {
    $sth = $this->db->prepare("SELECT * FROM users WHERE cardid=:cardid");
    $sth->bindParam('cardid', $args['cardid']);
    $sth->execute();
	if ($sth->rowCount() > 0) 
	{
		$user = $sth->fetchObject();
		return $this->response->withJson($user);
	}
	else
	{
		return $this->response->withJson(array('error' => 'Unknown card ID'));
	}
});

// Add a new user
$app->post('/user', function ($request, $response, $args) {
    $input = $request->getParsedBody();
    
    if($request->getAttribute('has_errors')) 
    {
        $sth = $this->db->prepare("SELECT * FROM users ORDER BY name");
        $sth->execute();
        $users = $sth->fetchAll();

        $message = "";
        //$message = var_dump($request->getAttribute('errors'));
        foreach($request->getAttribute('errors') as $field => $errors)
        {
            $message .= "<strong>".$field."</strong>:<ul>";
            foreach($errors as $error)
            {
                $message .= "<li>".$error."</li>";
            }
            $message .= "</ul>";
        }     
        
        return $this->view->render($response, 'index.html', [
            'users' => $users,
            'message' => $message,
            'type' => 'danger',
            'name' => $input['name'],
            'cardid' => $input['cardid'],
            'password' => $input['password'],
        ]);
    }
    else 
    {
        $sql = "INSERT INTO users (id, name, cardid, password) VALUES (default, :name, :cardid, :password)";
        $sth = $this->db->prepare($sql);
        
        $sth->bindParam('name', $input['name']);
        $sth->bindParam('cardid', $input['cardid']);
		$encryptedPassword = xorBase64($input['password']);
        $sth->bindParam('password', $encryptedPassword);
        
        $sth->execute();
        
        $sth = $this->db->prepare("SELECT * FROM users ORDER BY name");
        $sth->execute();
        $users = $sth->fetchAll();

        return $this->view->render($response, 'index.html', [
            'users' => $users,
            'message' => "User ".$input['name']." added!",
            'type' => 'success'
        ]);
    }
    
})->setName('add-user');

// Delete a user with given id
$app->delete('/user/{id}', function ($request, $response, $args) 
{
    $sth = $this->db->prepare("DELETE FROM users WHERE id=:id");
    $sth->bindParam('id', $args['id']);
    $sth->execute();
    
    $sth = $this->db->prepare("SELECT * FROM users ORDER BY name");
    $sth->execute();
    $users = $sth->fetchAll();
    
    return $this->view->render($response, 'index.html', [
            'users' => $users,
            'message' => "User ".$args['id']." deleted!",
            'type' => 'success'
        ]);
})->setName('delete-user');


// Update user form
$app->get('/user/update/{id}', function ($request, $response, $args) {
    $sth = $this->db->prepare("SELECT * FROM users WHERE id=:id");
    $sth->bindParam('id', $args['id']);
    $sth->execute();
    $user = $sth->fetchObject();
	
    return $this->view->render($response, 'update-user.html', [
			'id' => $user->id,
            'name' => $user->name,
            'cardid' => $user->cardid
        ]);
})->setName('update-user-form');


// Update user with given id
$app->put('/user/{id}', function ($request, $response, $args) {
    $input = $request->getParsedBody();
	
	if($request->getAttribute('has_errors')) 
    {
		$message = "";
        foreach($request->getAttribute('errors') as $field => $errors)
        {
            $message .= "<strong>".$field."</strong>:<ul>";
            foreach($errors as $error)
            {
                $message .= "<li>".$error."</li>";
            }
            $message .= "</ul>";
        }     
        
        return $this->view->render($response, 'update-user.html', [
            'message' => $message,
            'type' => 'danger',
			'id' => $args['id'],
            'name' => $input['name'],
            'cardid' => $input['cardid']
        ]);
	}
	else
	{
		$sql = "UPDATE users SET name=:name, cardid=:cardid, password=:password WHERE id=:id";
		$sth = $this->db->prepare($sql);
		
		$sth->bindParam('id', $args['id']);
		$sth->bindParam('name', $input['name']);
		$sth->bindParam('cardid', $input['cardid']);
		$encryptedPassword = xorBase64($input['password']);
		$sth->bindParam('password', $encryptedPassword);
		
		$sth->execute();
		
		$sth = $this->db->prepare("SELECT * FROM users ORDER BY name");
		$sth->execute();
		$users = $sth->fetchAll();
		
		return $this->view->render($response, 'index.html', [
            'users' => $users,
            'message' => "User ".$input['name']." updated!",
            'type' => 'success'
        ]);
	}  
    
})->setName('update-user');


// -------------------------------------------------- //

function xorBase64($text) 
{
    $settings = $c->get('settings')['lancer'];
	$key = $settings['lancer'];
    
    $i = 0;
    $encrypted = '';
    foreach (str_split($text) as $char) {
        $encrypted .= chr(ord($char) ^ ord($key{$i++ % strlen($key)}));
    }
    return base64_encode($encrypted);
}