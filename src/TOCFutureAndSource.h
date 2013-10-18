#import <Foundation/Foundation.h>
#import "TOCCancelTokenAndSource.h"

/*!
 * The states that a future can be in.
 */
enum TOCFutureState {
    /*!
     * The future has not completed or failed.
     * It will never complete or fail.
     *
     * @discussion Immortal futures eternally incomplete.
     * They are stable and unchanging.
     * They are guaranteed to remain incomplete until deallocated.
     *
     * All completion continuations/handlers given to futures in this state will be discarded without being run.
     *
     * A future becomes immortal when its source is deallocated without having set the future.
     *
     * A future can also become immortal due to a flattening cycle being detected, such as when a future is set to be its own result.
     */
    TOCFutureState_Immortal = 0,

    /*!
     * The future's source can still accept a result or failure.
     * The future is (perhaps temporarily) incomplete.
     *
     * @discussion All completion continuations/handlers given to futures in this state will be stored.
     * They will be run when the future completes or fails. or they will be discarded when the future becomes eternally incomplete.
     *
     * Note that the state of a future that can still be completed is volatile.
     * While you checked that a future could still be completed, it may have already transitioned to being completed, failed, or eternally incomplete.
     *
     * AbleToBeSet futures transition to the CompletedWithResult state when their source is given a (non-future) result.
     *
     * AbleToBeSet futures transition to the Flattening state when their source is given a result that is itself a future.
     *
     * AbleToBeSet futures transition to the Failed state when their source is given a failure.
     */
    TOCFutureState_AbleToBeSet = 1,
    
    /*!
     * The future has completed with a result.
     *
     * @discussion Completed futures are stable and unchanging.
     * They are guaranteed to remain completed with the same result until deallocated.
     *
     * All completion continuations/handlers given to futures in this state will be run immediately.
     * (Except failure handlers, which will of course be immediately discared instead of being run.)
     */
    TOCFutureState_CompletedWithResult = 2,

    /*!
     * The future has failed.
     *
     * @discussion Failed futures are stable and unchanging.
     * They are guaranteed to remain failed with the same failure value until deallocated.
     *
     * All completion continuations/handlers given to futures in this state will be run immediately.
     * (Except failure handlers, which will of course be immediately discared instead of being run.)
     */
    TOCFutureState_Failed = 3,

    /*!
     * The future has been set to a result, but that result is a future that is not yet complete.
     *
     * @discussion All completion continuations/handlers given to futures in this state will be stored.
     * They will be run when the future completes or fails, or they will be discarded if the future becomes eternally incomplete.
     */
    TOCFutureState_Flattening = 4,
    
};

@class TOCFuture;

/*!
 * The type of block passed to TOCFuture's finallyDo method.
 * The future given to the block is required to have succeeded or failed (i.e. to not be incomplete).
 */
typedef void (^TOCFutureFinallyHandler)(TOCFuture * completed);
/*!
 * The type of block passed to TOCFuture's thenDo method.
 * The value given to the block is a future's result.
 */
typedef void (^TOCFutureThenHandler)(id value);
/*!
 * The type of block passed to TOCFuture's catchDo method.
 * The value given to the block is a future's failure.
 */
typedef void (^TOCFutureCatchHandler)(id failure);

/*!
 * The type of block passed to TOCFuture's finally method.
 * The future given to the block is required to have succeeded or failed (i.e. to not be incomplete).
 */
typedef id (^TOCFutureFinallyContinuation)(TOCFuture * completed);
/*!
 * The type of block passed to TOCFuture's then method.
 * The value given to the block is a future's result.
 */
typedef id (^TOCFutureThenContinuation)(id value);
/*!
 * The type of block passed to TOCFuture's catch method.
 * The value given to the block is a future's failure.
 */
typedef id (^TOCFutureCatchContinuation)(id failure);

/*!
 * An eventual value that either succeeds with a result, or fails with a failure.
 *
 * @discussion TOCFuture is thread-safe.
 * It can be accessed from multiple threads concurrently.
 *
 * TOCFuture is auto-collapsing/flattening.
 * Any TOCFuture that would have had a result of type TOCFuture gets flattened and effectively becomes its result.
 * For example, [TOCFuture futureWithResult:[TocFuture futureWithResult:@1]] is equivalent to [TocFuture futureWithResult:@1].
 *
 * Note that automatic flattening does not apply to failures.
 * A TOCFuture's failure may be a TOCFuture.
 *
 * Use the then/catch/finally methods to continue with a block once a TOCFuture has a value.
 * They each return a TOCFuture for the continuation's completion, allowing you to chain computations together.
 *
 * You can use isIncomplete/hasResult/hasFailed to determine if the future has already completed or not.
 * Use forceGetResult/forceGetFailure to get the future's result or failure, or an exception if the future is in the wrong state.
 *
 * Use the TOCFutureSource class to control your own TOCFuture instances.
 */
@interface TOCFuture : NSObject

/*!
 * Returns a completed future containing the given result.
 *
 * @param resultValue The result for the returned future.
 * Allowed to be nil.
 * Allowed to be a future.
 *
 * @result A future containing the given result or else, if the given result is a future, collapses to returning the given result.
 *
 * @discussion
 * When the given resultValue is a future, automatic collapse makes the returned future equivalent to the given future.
*/
+(TOCFuture *)futureWithResult:(id)resultValue;

/*!
 * Returns a failed future containing the given failure.
 *
 * @param failureValue The failure for the returned future.
 * Allowed to be nil.
 * Allowed to be a future.
 *
 * @result A future containing the given failure.
 *
 * @discussion
 * Note that automatic collapse does not apply to failures.
 * The returned future will not be flattened even if the given failure is itself a future.
 */
+(TOCFuture *)futureWithFailure:(id)failureValue;


/*!
 * Returns a cancel token that is cancelled when the receiving future has succeeded with a result or failed.
 */
-(TOCCancelToken*) cancelledOnCompletionToken;

/*!
 * Returns the current state of the receiving future: completed-with-result, failed, still-completable, or eternally-incomplete.
 *
 * @discussion Note that the state of a future that can still be completed is volatile.
 * While you checked that the future was still completable, it may have already transitioned to being completed-with-result, failed, or eternally-incomplete.
 */
-(enum TOCFutureState) state;

/*!
 * Determines if the receiving future has not yet completed or failed.
 *
 * @discussion Futures in both the StillCompletable and EternallyIncomplete states are considered incomplete.
 */
-(bool)isIncomplete;

/*!
 * Determines if the receiving future has completed with a result, as opposed to having failed or still being incomplete.
 */
-(bool)hasResult;

/*!
 * Determines if the receiving future has failed, as opposed to having completed with a result or still being incomplete.
 */
-(bool)hasFailed;

/*!
 * Determines if the receiving future has failed with a cancellation token as its failure value.
 */
-(bool)hasFailedWithCancel;

/*!
 * Determines if the receiving future has failed with a TOCTimeout instance as its failure value.
 */
-(bool)hasFailedWithTimeout;

/*!
 * Returns the receiving future's result, unless it doesn't have one in which case an exception is raised.
 *
 * @pre hasResult must be true
 *
 * @result The future's result.
 *
 * @discussion
 * Raises an exception immediately, without waiting, if the future is still incomplete.
 */
-(id)forceGetResult;

/*!
 * Returns the receiving future's failure, unless it doesn't have one in which case an exception is raised.
 *
 * @pre hasFailed must be true
 *
 * @result The future's failure.
 *
 * @discussion
 * Raises an exception immediately, without waiting, if the future is still incomplete.
 */
-(id)forceGetFailure;

/*!
 * Eventually runs a 'finally' handler on the receiving future, once it has completed with a result or failed, unless cancelled.
 *
 * @param completionHandler The block to run once the future has completed, unless the cancel token is cancelled first.
 *
 * @param unlessCancelledToken If this token is cancelled before the future completes, the completion handler will be discarded instead of run.
 * A nil cancel token corresponds to an immortal cancel token.
 *
 * @discussion If the receiving future has already failed or completed with a result, the handler is run inline.
 *
 * If the receiving future has already completed and the given cancel token has already been cancelled, the cancellation "wins": the handler is not run.
 *
 * Cancelling a callback causes it to be immediately discarded.
 * The future will no longer reference it, and it may be deallocated.
 */
-(void) finallyDo:(TOCFutureFinallyHandler)completionHandler
           unless:(TOCCancelToken*)unlessCancelledToken;

/*!
 * Eventually runs a 'then' handler on the receiving future's result, unless cancelled.
 *
 * @param resultHandler The block to run when the future succeeds with a result.
 *
 * @discussion If the receiving future has already succeeded with a result, the handler is run inline.
 *
 * If the receiving future fails, instead of succeeding with a result, the handler is not run.
 *
 * If the given cancel token is already cancelled and the receiving future already has a result, the cancel token "wins": the handler is not run.
 *
 * Cancelling a callback causes it to be immediately discarded.
 * The future will no longer reference it, and it may be deallocated.
 */
-(void) thenDo:(TOCFutureThenHandler)resultHandler
        unless:(TOCCancelToken*)unlessCancelledToken;

/*!
 * Eventually runs a 'catch' handler on the receiving future's failure, unless cancelled.
 *
 * @discussion If the receiving future has already failed, the handler is run inline.
 *
 * @param failureHandler The block to run when the future fails.
 *
 * @param unlessCancelledToken If this token is cancelled before the future completes, the completion handler will be discarded instead of run.
 * A nil cancel token corresponds to an immortal cancel token.
 *
 * If the receiving future succeeds with a result, instead of failing, the handler is not run.
 *
 * If the receiving future has already failed and the given cancel token has already been cancelled, the cancellation "wins": the handler is not run.
 *
 * Cancelling a callback causes it to be immediately discarded.
 * The future will no longer reference it, and it may be deallocated.
 */
-(void) catchDo:(TOCFutureCatchHandler)failureHandler
         unless:(TOCCancelToken*)unlessCancelledToken;

/*!
 * Eventually evaluates a 'finally' continuation on the receiving future, once it has completed with a result or failed.
 *
 * @param completionContinuation The block to evaluate when the future completes with a result or fails.
 *
 * @param unlessCancelledToken If this token is cancelled before the receiving future completes, the continuation is cancelled.
 * The resulting future will immediately transition to the failed state, with the given token as its failure value.
 * The cancel token may be nil, in which case it acts like a cancel token that is never cancelled.
 *
 * @result A future for the eventual result of evaluating the given 'finally' block on the receiving future once it has completed.
 * If the given cancellation token is cancelled before the receiving future completes, the resulting future immediately fails with the cancellation token as its failure.
 *
 * @discussion If the receiving future has already completed, the continuation is run inline.
 *
 * If the continuation returns a future, instead of a normal value, then this method's result is automatically flattened to match that future instead of containing it.
 *
 * If the receiving future has already completed and the given cancel token has already been cancelled, the cancellation "wins": the continuation is not run.
 *
 * Cancelling a callback causes it to be immediately discarded.
 * The future will no longer reference it, and it may be deallocated.
 */
-(TOCFuture *)finally:(TOCFutureFinallyContinuation)completionContinuation
               unless:(TOCCancelToken*)unlessCancelledToken;

/*!
 * Eventually evaluates a 'then' continuation on the receiving future's result, or else propagates the receiving future's failure.
 *
 * @result A future for the eventual result of evaluating the given 'then' block on the receiving future's result, or else a failure if the receiving future fails.
 * If the given cancellation token is cancelled before the receiving future completes, the resulting future immediately fails with the cancellation token as its failure.
 *
 * @param resultContinuation The block to evaluate when the future succeeds with a result.
 *
 * @param unlessCancelledToken If this token is cancelled before the receiving future completes, the continuation is cancelled.
 * The resulting future will immediately transition to the failed state, with the given token as its failure value.
 * The cancel token may be nil, in which case it acts like a cancel token that is never cancelled.
 *
 * @discussion If the receiving future has already succeeded with a result, the continuation is run inline.
 *
 * If the receiving future fails, instead of succeeding with a result, the continuation is not run and the failure is propagated into the returned future.
 *
 * If the continuation returns a future, instead of a normal value, then this method's result is automatically flattened to match that future instead of containing it.
 *
 * If the given cancel token is already cancelled and the receiving future already has a result, the cancel token "wins": the continuation is not run.
 *
 * Cancelling a callback causes it to be immediately discarded.
 * The future will no longer reference it, and it may be deallocated.
 */
-(TOCFuture *)then:(TOCFutureThenContinuation)resultContinuation
            unless:(TOCCancelToken*)unlessCancelledToken;

/*!
 * Eventually matches the receiving future's result, or else evaluates a 'catch' continuation on the receiving future's failure, unless cancelled.
 *
 * @param resultContinuation The continuation to evaluate when the future fails.
 *
 * @param unlessCancelledToken If this token is cancelled before the receiving future fails, the continuation is cancelled.
 * The resulting future will immediately transition to the failed state, with the given token as its failure value.
 * The cancel token may be nil, in which case it acts like a cancel token that is never cancelled.
 *
 * @result A future for the eventual result of the receiving future, or else the eventual result of running the receiving future's failure through the given 'catch' block.
 * If the given cancellation token is cancelled before the receiving future completes, the resulting future immediately fails with the cancellation token as its failure.
 *
 * @discussion If the receiving future has already failed, the continuation is run inline.
 *
 * If the continuation returns a future, instead of a normal value, then this method's result is automatically flattened to match that future instead of containing it.
 *
 * If the receiving future has already failed and the given cancel token has already been cancelled, the cancellation "wins": the continuation is not run.
 *
 * Cancelling a callback causes it to be immediately discarded.
 * The future will no longer reference it, and it may be deallocated.
 */
-(TOCFuture *)catch:(TOCFutureCatchContinuation)failureContinuation
             unless:(TOCCancelToken*)unlessCancelledToken;

@end

/*!
 * Creates and controles a TOCFuture.
 *
 * @discussion
 * TOCFutureSource is thread-safe.
 * It can be accessed and controlled from multiple threads concurrently.
 *
 * Use trySetResult/trySetFailure to cause the future to complete with a result or fail with a failure.
 *
 * If a future source is deallocated before its future completes, its future becomes immortal.
 * Immortal futures never complete with a result or failure, and discard their callbacks without running them (to allow dealloc to occur).
 */
@interface TOCFutureSource : NSObject

/*!
 * Returns the future controlled by the receiving future source.
 */
@property (readonly, nonatomic) TOCFuture* future;

/*!
 * Attempts to set the receiving future source to complete with the given result.
 *
 * @result True when the future source was successfully set, and false when it was already set.
 *
 * @param finalResult The result the receiving future source should complete with.
 * Allowed to be nil.
 * Allowed to be a future.
 *
 * @discussion If the receiving future source has already been set, this method has no effect (and returns false).
 *
 * If the given result is a future and this method succeeds, then the receiving future source will collapse to match the future instead of containing it.
 *
 * When the future source is set to match an incomplete future, it remains incomplete (but still set) until that future completes.
 *
 * If you try to make set a future source's result to its own future, its future becomes immortal and discards all callbacks.
 */
-(bool) trySetResult:(id)result;

/*!
 * Attempts to set the receiving future source to fail with the given failure.
 *
 * @result True when the future source was successfully set, and false when it was already set.
 *
 * @param finalFailure The failure the receiving future source should fail with.
 * Allowed to be nil.
 * Allowed to be a future.
 *
 * @discussion If the receiving future source has already been set, this method has no effect (and returns false).
 *
 * No automatic collapse occurs when the given failure is a future. The receiving future source will just contain a failure that is a future, instead of matching that future.
 */
-(bool) trySetFailure:(id)failure;

/*!
 * Sets the receiving future source to complete with the given result, or else raises an exception if it was already set.
 *
 * @param finalResult The result the receiving future source should complete with.
 * Allowed to be nil.
 * Allowed to be a future.
 *
 * @discussion If the receiving future source has already been set, this method has no effect but raises an exception.
 *
 * If the given result is a future and this method succeeds, then the receiving future source will collapse to match the future instead of containing it.
 *
 * When the future source is set to match an incomplete future, it remains incomplete (but still set) until that future completes.
 */
-(void) forceSetResult:(id)result;

/*!
 * Sets the receiving future source to fail with the given failure, or else raises an exception.
 *
 * @param finalFailure The failure the receiving future source should fail with.
 * Allowed to be nil.
 * Allowed to be a future.
 *
 * @discussion If the receiving future source has already been set, this method has no effect but raises an exception.
 *
 * No automatic collapse occurs when the given failure is a future. The receiving future source will just contain a failure that is a future, instead of matching that future.
 */
-(void) forceSetFailure:(id)failure;

/*!
 * Attempts to make the receiving source's future fail with a cancellation token as its failure value.
 *
 * @result True when the future was successfully set, false when the future was already set.
 *
 * @discussion If the receiving future source has already been set, this method has no effect (and returns false).
 */
-(bool) trySetFailedWithCancel;

/*!
 * Forces the receiving future source to fail with a cancellation token as its failure value.
 *
 * @discussion If the receiving future source has already been set, this method has no effect but raises an exception.
 */
-(void) forceSetFailedWithCancel;

/*!
 * Attempts to make the receiving source's future fail with an instance of TOCTimeout as its failure value.
 *
 * @result True when the future was successfully set, false when the future was already set.
 *
 * @discussion If the receiving future source has already been set, this method has no effect (and returns false).
 */
-(bool) trySetFailedWithTimeout;

/*!
 * Forces the receiving future source to fail with an instance of TOCTimeout as its failure value.
 *
 * @discussion If the receiving future source has already been set, this method has no effect but raises an exception.
 */
-(void) forceSetFailedWithTimeout;

/*!
 * Creates and returns a future source that is dependent on the given cancel token.
 * If the cancel token is cancelled, and the future source has not set its future yet, its future is automatically cancelled.
 *
 * @param untilCancelledToken The token that causes the resulting source's future to fail with a cancellation, if it wasn't set yet.
 * Allowed to be nil, in which case the returned future source is just a normal future source.
 *
 * @result A future source that depends on the given cancel token.
 */
+(TOCFutureSource*) futureSourceUntil:(TOCCancelToken*)untilCancelledToken;

@end
