# mwe_cpsr
this repository is designed to test the client prediction and server reconciliation algorithm in an isolated place so that we can verify it works properly, thus allowing for easier debugging.

## the setup
Suppose you have a client and server setup, our goal is to give the player the highest degree of freedom to express themselves in the game, in a game with more game updates or ticks per second, you can be more expressive because whenever you try and do something, the game will pick up on that faster. 

Our goal is to create a cpsr system which can allow the client to give the user a crisp experience where they will feel that they are taking advantage of their hardware, and attempt to actually make that a reality, the reason why making that a reality is harder than making it feel that way is because there are solutions to this problem which only give the user the illusion of more control.

Specifically we'll make the client and server both have a main loop which runs at 512 hz, we want to take advantage of this frequency on the client, but at the same time we don't want to send out at 512 hz, because I believe that is too fast (needs to be tested). 

## the algorithm
the cpsr algorithm is a simple algorithm in theory which clobbers a clients state with the authorative server state and the corrects for this by reapplying client inputs.


## client and server need similar time deltas

Initially the algorithm seems simple until you actually start understanding how inputs are processed, usually a single input is combined with a delta time to produce new game state via what we call the process function, even if both the client and server theoreritically are running at the same rate, the measured deltas between ticks will be different on the client and server, thus an input processed on the client resulting in some game state will most likely yield a game state with a slight difference on the server because the servers measured delta time will be different. 

This shows us that given the same input on two different machines, if we want the resulting game state to be as similar as possible between the two machines we must be sure to keep the time deltas as similar as possible.

## isn't that a problem

We already mentioned that we want to take advantage of the 512Hz on the client, one idea is that we sample the input at this rate, and combine them each with their own unique time deltas which will be approximately 1/512 seconds, if we do so then we will be processing our deltas on the client at a much faster rate, and with much smaller deltas. 

The first thing to note is that since we're only sending out at 60hz then on average about 8 of these inputs would have to be sent out all together, this in itself can be problematic, because once those get to the server we have to decide what we're going to do with them. 

The first problem is that if we just send them over to the server in a bundle, how do we unpack this bundle, do we consume one off the top for each server tick? This would theoretically work since the production and consume rates are equal, but then the server would "catch up" at a much slower rate to what the current game state would be on the client, thus making the game feel laggy, instead it might make more sense to process them all at once. In either of the discussed cases, we have to make a bad decision, we either have to delay the server state, or have a protocol in place when multiple updates come in at once.

If we assume the client and server are ticking at the same rate, then there is still a problem, if we send over our updates in update, time delta pairs, then when a bundle of those come along we could just run the process function on each one on the server side right? 

While this might make some sense when you have one player connected, this is making the client more authorative then it should be, so even in the single user context this doesn't make sense, then in the multi-user context it makes a lot less sense because the server would be processing and moving along to states that would be in the future, as compared to just updating 

## update isolation

Another consideration that has to be made in such a system is how the process function operates, if your system is a physics engine, then when you tick the physics engine its usually the case that all other entities get moved along in time, so that if you were to pass in a bunch of update, time deltas for each client when the server would process those it would inadvertantly process all the other entities moving them along in time, causing unintended effects. 

In order to even consider the solution of passing the update, time delta pairs you need to be sure that the system you're updating allows for isolation based updates.

## the way you update has an effect

When running the cpsr algorithm there are many types of data that you might want to run cpsr on, the type of data you want to run it on depends on what type of solutions there 


## delay and interpolate
The first approach you could do to take advantage of the 512hz is to 

, so instead suppose we send out our messages at a rate of 60hz so we update our players position at 512 hz using the isolated physics tick update on that player, additionally we can batch the players inputs together into larger updates, we do this by collecting up updates that 
